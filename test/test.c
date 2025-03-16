//#include "../src/C/db.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define BUFSIZE 4096

HANDLE hChildStdinWr = NULL; // Parent process writes commands in this variable
HANDLE hChildStdoutRd = NULL; // Parent process reads outputs from this variable
PROCESS_INFORMATION pi;

// This function creates the child process and configures the pipes

BOOL CreateChildProcess(const char* program) {
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    HANDLE hChildStdoutWr, hChildStdinRd;

    // Create a pipe for child's STDOUT (parent reads)
    if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &sa, 0)) {
        fprintf(stderr, "CreatePipe (stdout) failed (%lu)\n", GetLastError());
        return FALSE;
    }
    
    // Ensure parent's read handle is not inherited by child
    SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);

    // Create pipe for child's STDIN (parent writes)
    if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &sa, 0)) {
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
BOOL CompareOutput(char** actual, int actualCount, const char** expected, int expectedCount) {
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
        "insert 1 user1 person1@example.com", "select", ".exit"
    };

    const char* expected[] = {
        "db > 'insert 1 user1 person1@example.com'. ",
        "Executed. ",
        "db > 'select'. ",
        "(1, user1, person1@example.com) ",
        "Executed. ", 
        "db > '.exit'. "
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

    // Validate Output
    BOOL success = CompareOutput(
        actualLines, actualCount,
        expected, sizeof(expected)/sizeof(expected[0])
    );

    //Clean up
    free(output);
    for (int i = 0; i < actualCount; i++) {free(actualLines[i]);}
    free(actualLines);
    return success;
}

// void test_command_read() {
//     FILE *fp;
//     char buffer[128];
//     const char *command = "dir";


//     fp = popen(command, "r");
//     if (fp==NULL) {
//         perror("popen failed. \n");
//         exit(EXIT_FAILURE);
//     }

//     while (fgets(buffer, sizeof(buffer), fp)!= NULL) {
//         printf("Recieved %s", buffer);
//     }

//     pclose(fp);
// }


int main(){
    // test_command_read();
    // return 0;
    if (!CreateChildProcess("db.exe")) return 1;

    BOOL testResult = TestInsertAndSelect();

    //Cleanup
    CloseHandle(hChildStdoutRd);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return testResult ? 0 : 1;
}