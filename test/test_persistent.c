//#include "../src/C/db.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <io.h>

#define BUFSIZE 262144

HANDLE hChildStdinWr = NULL; // Parent process writes commands in this variable
HANDLE hChildStdoutRd = NULL; // Parent process reads outputs from this variable
PROCESS_INFORMATION pi;
DWORD desiredBufferSize = 65536;

// This function creates the child process and configures the pipes

BOOL CreateChildProcess(const char* program) {
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    HANDLE hChildStdoutWr, hChildStdinRd;

    // Create a pipe for child's STDOUT (parent reads)
    if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &sa, desiredBufferSize)) {
        fprintf(stderr, "CreatePipe (stdout) failed (%lu)\n", GetLastError());
        return FALSE;
    }
    
    // Ensure parent's read handle is not inherited by child
    SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);

    // Create pipe for child's STDIN (parent writes)
    if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &sa, desiredBufferSize)) {
        fprintf(stderr, "Create pipe (stdin) failed (%lu)\n", GetLastError());
        return FALSE;
    }

    // Ensure parent's write handle is not inherited by child
    SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0);

    // Configure child's handles
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hChildStdinRd;
    si.hStdOutput = hChildStdoutWr;
    si.hStdError = hChildStdoutWr;

    // Spawn child processes
    BOOL success = CreateProcess(
        NULL, (LPSTR)program, NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi
    );

    if (!success) {
        fprintf(stderr, "CreateProcess failed (%lu)\n", GetLastError());
        return FALSE;
    }

    // Close unused handles (child's end)
    CloseHandle(hChildStdoutWr);
    CloseHandle(hChildStdinRd);
    return TRUE;

}

// Writes a command to the child's stdin followed by a new line
BOOL SendCommand(const char* command) {
    DWORD bytesWritten;
    BOOL success = WriteFile(hChildStdinWr, command, strlen(command), &bytesWritten, NULL);
    success &= WriteFile(hChildStdinWr, "\n", 1, &bytesWritten, NULL);
    return success;
}

// Reads all outputs from child until EOF
char* ReadAllOutput() {
    char buffer[BUFSIZE];
    DWORD bytesRead;
    char* output = malloc(1);
    output[0] = '\0';

    while (ReadFile(hChildStdoutRd, buffer, BUFSIZE-1,  &bytesRead, NULL)&& bytesRead>0) {
        buffer[bytesRead] = '\0';
        output = realloc(output, strlen(output)+bytesRead+1);
        strcat(output, buffer);
    }
    return output;

}

char* ReadLastOutput() {
    //First read all output
    char* allOutput = ReadAllOutput();

    if (allOutput[0] == '\0') {
        // Empty output
        return allOutput;
    }

    //Find the last line
    char* lastline =allOutput;
    char* ptr = allOutput;

    while (*ptr) {
        if (*ptr == '\n') {
            lastline = ptr + 1;
        }
        ptr++;
    }

    //Create a copy of just the last line
    char* result = _strdup(lastline);

    //Free the full output buffer
    free(allOutput);

    return result;
}

// Splits output into lines (handles \r\n and \n)
int SplitOutputLines(char* output, char*** lines) {
    int count = 0;
    char* line = strtok(output, "\r\n");
    *lines = malloc(sizeof(char*)*BUFSIZE); // Max expected lines
    
    while (line != NULL) {
        (*lines)[count++] = _strdup(line);
        line = strtok(NULL, "\r\n");
    }

    return count;
}

// Compares actual output lines with expected output lines
BOOL CompareOutput(char** actual, int actualCount, char** expected, int expectedCount) {
    if (actualCount != expectedCount) {
        fprintf(stderr, "Line count mismatch: %d vs %d\n", actualCount, expectedCount);
        return FALSE;
    }

    for (int i=0; i < actualCount; i++) {
        if (strcmp(actual[i], expected[i])!= 0) {
            fprintf(stderr, "Mismatch at line %d:\nExpected: '%s'\nActual: '%s'\n",i, expected[i], actual[i]);
            return FALSE;
        }
    }
    return TRUE;
}



// Test case (insert and retrieve a row)
BOOL TestInsertAndSelect() {
    const char* commands[] = {
        "insert 1 user1 person1@example.com", 
        ".exit"
    };

    char* expected[]={
        "db > Executed. ",
        "db > "
    };

    // Send commands to child
    for (int i=0; i < sizeof(commands)/sizeof(commands[0]); i++) {
        if (!SendCommand(commands[i])) {
            fprintf(stderr, "Failed to send command: %s\n", commands[i]);
            return FALSE;
        }
    }

    //Close input pipe to signal EOF
    CloseHandle(hChildStdinWr);


    //Read and parse output
    char* output = ReadAllOutput();
    char** actualLines;
    int actualCount = SplitOutputLines(output, &actualLines);
    // printf("Total Output:\n" );
    // for (int i; i<actualCount; i++) {
    //     printf("%d  %s\n", i, actualLines[i]);
    // }

    // Validate Output

    BOOL success = CompareOutput(
        actualLines, actualCount,
        expected, sizeof(expected)/sizeof(char *)
    );


    //Clean up
    free(output);
    for (int i = 0; i < actualCount; i++) {free(actualLines[i]);}
    free(actualLines);
    return success;
}

BOOL TestInsertAndSelectPersist() {
    const char* commands[] = {
        "select", 
        ".exit"
    };

    char* expected[]={
        "db > (1, user1, person1@example.com) ",
        "Executed. ",
        "db > "
    };

    // Send commands to child
    for (int i=0; i < sizeof(commands)/sizeof(commands[0]); i++) {
        if (!SendCommand(commands[i])) {
            fprintf(stderr, "Failed to send command: %s\n", commands[i]);
            return FALSE;
        }
    }

    //Close input pipe to signal EOF
    CloseHandle(hChildStdinWr);


    //Read and parse output
    char* output = ReadAllOutput();
    char** actualLines;
    int actualCount = SplitOutputLines(output, &actualLines);
    // printf("Total Output:\n" );
    // for (int i; i<actualCount; i++) {
    //     printf("%d  %s\n", i, actualLines[i]);
    // }

    // Validate Output

    BOOL success = CompareOutput(
        actualLines, actualCount,
        expected, sizeof(expected)/sizeof(char *)
    );


    //Clean up
    free(output);
    for (int i = 0; i < actualCount; i++) {free(actualLines[i]);}
    free(actualLines);
    return success;
}
int main(){
    // test_command_read();
    // return 0;

    if(remove("test.db")==0) {
        printf("The file was deleted successfully.\n");
    } else {
        printf("The was not deleted.\n");
    }

    if (!CreateChildProcess("db.exe test.db")) return 1;

    BOOL testResult = TestInsertAndSelect();
    if (testResult) {
        printf("The test of insertion is successful.\n");
    }
    else {
        printf("The test has failed");
    }

    //Cleanup
    CloseHandle(hChildStdoutRd);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);


    if (!CreateChildProcess("db.exe test.db")) return 1;
    
    BOOL testResultPersist = TestInsertAndSelectPersist();
    if (testResultPersist) {
        printf("The test of persistence is successful.\n");
    }
    else {
        printf("The test has failed");
    }

    //Cleanup
    CloseHandle(hChildStdoutRd);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);


    return testResult && testResultPersist ? 0 : 1;
}