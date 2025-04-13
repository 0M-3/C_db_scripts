//#include "../src/C/db.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

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
        "insert 1 user1 person1@example.com", "select"
        //,".exit"
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

    
    char long_user[32]; 
    char long_email[320];

    for (int i=0; i<12; i++) {
        long_user[i]='a';
    }
    long_user[12] = '\0';

    for (int i=0; i<255; i++) {
        long_email[i]='a';
    }
    long_email[255] = '\0';

    char command2[400];
    sprintf(command2, "insert 593829 %s %s", long_user, long_email);
    printf("long user: %s", long_user);
    printf("long email: %s", long_email);

    if (!SendCommand(command2)){
        fprintf(stderr, "Failed to send command: %s\n", command2);
    }

    char repeatCommand[64];
        
    for (int j=0; j<2000; j++) {
        sprintf(repeatCommand, "insert %d user%d person%d@example.com", j, j, j);
        if(!SendCommand(repeatCommand)) {
            fprintf(stderr, "Failed to send Command: %s\n", repeatCommand);
        }
        // printf("Sent command: %s\n", repeatCommand);
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
        expected, sizeof(expected)/sizeof(expected[0])
    );

/*
    char repeatCommand[128];
    char expectedFinal[] = "Error: Table full. ";
    char* repeatOutput = NULL;
    int i = 1;

    do {
        sprintf(repeatCommand, "insert %d user%d person%d@example.com", i,i,i);
        if(!SendCommand(repeatCommand)) {
            printf("%s\n", repeatCommand);
            fprintf(stderr, "Failed to send command: %s\n", repeatCommand);
            break;
        }
        i++;
        repeatOutput = ReadLastOutput();
    } while (strcmp(repeatOutput, expectedFinal)!=0);

*/
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