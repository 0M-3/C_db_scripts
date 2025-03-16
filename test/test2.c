#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>


#define MAX_OUTPUT_LINE_LENGTH 1024
#define MAX_OUTPUT_LINES 100


char** run_script(char** commands, int command_count, int* output_line_count) {
    FILE* pipe = popen("./db", "r");
    if (!pipe) {
        perror("Failed to open pipe to db");
        exit(EXIT_FAILURE);
    }

    char** output = malloc(MAX_OUTPUT_LINES * sizeof(char*));
    if (!output) {
        perror("Memory allocation failed");
        pclose(pipe);
        exit(EXIT_FAILURE);
    }

    // Read output line by line
    char line[MAX_OUTPUT_LINE_LENGTH];
    int line_count = 0;
 
    for (int i=0; i<command_count; i++) {
        if (fprintf(pipe, "%s\n", commands[i])<0 ) {
            perror("Error:Output is empty. Failed to write command to pipe");
        }

        // Remove trailing newline
        size_t len = strlen(line);

        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        output[line_count] = strdup(line);
        if (!output[line_count]) {
            perror("Memory allocation failed");
            exit(1);
        }
            
        line_count++;

    }
    // Close pipe
    pclose(pipe);
    
    *output_line_count = line_count;
    return output;
}

char** exec_script(char** commands, int command_count, int* output_line_count) {
    int pipe_to_child[2];    // Parent writes commands here (child's STDIN)
    int pipe_from_child[2];  // Child writes output here (parent's STDOUT)

    if (pipe(pipe_to_child) == -1) {
        perror("pipe (to child) failed");
        exit(EXIT_FAILURE);
    }
    if (pipe(pipe_from_child) == -1) {
        perror("pipe (from child) failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        // Redirect STDIN: use the reading end of pipe_to_child
        if (dup2(pipe_to_child[0], STDIN_FILENO) == -1) {
            perror("dup2 for STDIN failed");
            exit(EXIT_FAILURE);
        }
        // Redirect STDOUT: use the writing end of pipe_from_child
        if (dup2(pipe_from_child[1], STDOUT_FILENO) == -1) {
            perror("dup2 for STDOUT failed");
            exit(EXIT_FAILURE);
        }

        // Close the original file descriptors that are no longer needed.
        close(pipe_to_child[0]);
        close(pipe_to_child[1]);
        close(pipe_from_child[0]);
        close(pipe_from_child[1]);

        // Execute the target executable ("./db")
        execl("./db", "./db", (char *)NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    }

    // Parent process:
    // Close the ends not used by the parent.
    close(pipe_to_child[0]);   // Parent writes to child's stdin.
    close(pipe_from_child[1]); // Parent reads from child's stdout.

    // Set up FILE streams to facilitate writing/reading.
    FILE *child_stdin = fdopen(pipe_to_child[1], "w");
    if (!child_stdin) {
        perror("fdopen for child_stdin failed");
        exit(EXIT_FAILURE);
    }
    FILE *child_stdout = fdopen(pipe_from_child[0], "r");
    if (!child_stdout) {
        perror("fdopen for child_stdout failed");
        exit(EXIT_FAILURE);
    }

    // Allocate an array for output strings.
    char **output = malloc(MAX_OUTPUT_LINES * sizeof(char *));
    if (!output) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    int line_count = 0;
    char line[MAX_OUTPUT_LINE_LENGTH];

    // For each command, send it to the child's stdin and then read one output line.
    for (int i = 0; i < command_count; i++) {
        if (fprintf(child_stdin, "%s\n", commands[i]) < 0) {
            perror("Error writing command to child process");
        }
        fflush(child_stdin);  // Ensure the command is sent immediately.

        // Read one line of response from the child's stdout.
        if (fgets(line, sizeof(line), child_stdout) != NULL) {
            // Remove the trailing newline character, if any.
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            output[line_count] = strdup(line);
            if (!output[line_count]) {
                perror("Memory allocation failed for output line");
                exit(EXIT_FAILURE);
            }
            line_count++;
        } else {
            // If no output is received, break out of the loop.
            break;
        }
    }

    // Signal the child that no further input will be sent.
    fclose(child_stdin);
    // Close the reading stream.
    fclose(child_stdout);

    // Wait for the child process to terminate.
    int status;
    waitpid(pid, &status, 0);

    *output_line_count = line_count;
    return output; 
}

// Function to check if a string is in an array of strings
bool string_in_array(const char* str, char** array, int array_size) {
    for (int i = 0; i < array_size; i++) {
        if (strcmp(str, array[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Function to compare two string arrays (regardless of order)
bool match_array(char** arr1, int arr1_size, char** arr2, int arr2_size) {
    if (arr1_size != arr2_size) {
        return false;
    }
    
    for (int i = 0; i < arr1_size; i++) {
        if (!string_in_array(arr1[i], arr2, arr2_size)) {
            return false;
        }
    }
    
    for (int i = 0; i < arr2_size; i++) {
        if (!string_in_array(arr2[i], arr1, arr1_size)) {
            return false;
        }
    }
    
    return true;
}

// Test for inserting and retrieving a row
bool test_insert_and_retrieve_row() {
    printf("Running test: inserts and retrieves a row\n");
    
    char* commands[] = {
        "insert 1 user1 person1@example.com",
        "select",
        ".exit"
    };
    

    char* expected_output[] = {
        "db > Executed.",
        "db > (1, user1, person1@example.com)",
        "Executed.",
        "db > "
    };
    
   
    int output_line_count = 0;
    char** output = exec_script(commands, 3, &output_line_count);
    
    bool result = match_array(output, output_line_count, 
                             expected_output, 4);
    
    printf("Test %s\n", result ? "PASSED" : "FAILED");
    
    // Print actual output for debugging
    if (!result) {
        printf("Expected output:\n");
        for (int i = 0; i < 4; i++) {
            printf("  '%s'\n", expected_output[i]);
        }
        printf("Actual output (%d lines):\n", output_line_count);
        for (int i = 0; i < output_line_count; i++) {
            printf("  '%s'\n", output[i]);
        }
    }
    
    // Free memory
    for (int i = 0; i < output_line_count; i++) {
        free(output[i]);
    }
    free(output);
    
    return result;
}

int main() {
    // Run tests
    bool all_tests_passed = true;
    
    all_tests_passed &= test_insert_and_retrieve_row();
    
    // Print summary
    printf("\nTest summary: %s\n", all_tests_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    return all_tests_passed ? 0 : 1;
}
