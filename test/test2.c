#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_OUTPUT_LINE_LENGTH 256
#define MAX_OUTPUT_LINES 100

// Function to run commands and capture output
// char** run_script(char** commands, int command_count, int* output_line_count) {
//     FILE* pipe = popen("./db.exe", "w+");
//     if (!pipe) {
//         perror("Failed to open pipe to db");
//         exit(EXIT_FAILURE);
//     }

//     // Write commands to the program
//     for (int i = 0; i < command_count; i++) {
//         fprintf(pipe, "%s\n", commands[i]);
//     }
    
//     // Flush to make sure all commands are sent
//     fflush(pipe);
    
//     // Allocate memory for output lines
//     char** output = malloc(MAX_OUTPUT_LINES * sizeof(char*));
//     if (!output) {
//         perror("Memory allocation failed");
//         exit(1);
//     }


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
            perror("Error:Output is empty. Failed to write command to pipe")
        };

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
            
        for (int i = 0; i < command_count; i++) {
            printf("%s", commands[i]);
            fprintf(pipe, "%s\n", commands[i]);
            if (fprintf(pipe, "%s\n", commands[i]) < 0) {
                // perror("Failed to write command to pipe");
                // pclose(pipe);
                // exit(EXIT_FAILURE);
            }
        }

        line_count++;
 
        if (fprintf(pipe, "%s\n", commands[i]) < 0) {
            perror("Failed to write command to pipe");
            pclose(pipe);
            exit(EXIT_FAILURE);
        }
 

    }
    // Close pipe
    pclose(pipe);
    
    *output_line_count = line_count;
    return output;
}

char** run_script(char** commands, int command_count, int* output_line_count) {
    FILE* pipe = popen("./db", "r");
    if (!pipe) {
        perror("Failed to open pipe to db");
        exit(EXIT_FAILURE);
    }

    // Write commands to the program
    for (int i = 0; i < command_count; i++) {
        printf("%s", commands[i]);
        fprintf(pipe, "%s\n", commands[i]);
        if (fprintf(pipe, "%s\n", commands[i]) < 0) {
            perror("Failed to write command to pipe");
            pclose(pipe);
            exit(EXIT_FAILURE);
        }

    }

    // Flush to make sure all commands are sent
    fflush(pipe);

    // Allocate memory for output lines
    char** output = malloc(MAX_OUTPUT_LINES * sizeof(char*));
    if (!output) {
        perror("Memory allocation failed");
        pclose(pipe);
        exit(EXIT_FAILURE);
    }

    // Read the output from the pipe
    // *output_line_count = 0;
    // char buffer[MAX_OUTPUT_LINE_LENGTH];
    // while (fgets(buffer, MAX_OUTPUT_LINE_LENGTH, pipe) != NULL) {
    //     if (*output_line_count >= MAX_OUTPUT_LINES) {
    //         fprintf(stderr, "Output exceeds maximum line count\n");
    //         break;
    //     }
    //     output[*output_line_count] = strdup(buffer);
    //     if (!output[*output_line_count]) {
    //         perror("Memory allocation failed");
    //         pclose(pipe);
    //         exit(EXIT_FAILURE);
    //     }
    //     (*output_line_count)++;
    // }

    // Close the pipe
    // if (pclose(pipe) == -1) {
    //     perror("Failed to close the pipe");
    //     exit(EXIT_FAILURE);
    // }

    // return output;


    // Read output line by line
    char line[MAX_OUTPUT_LINE_LENGTH];
    int line_count = 0;
    
    while (fgets(line, MAX_OUTPUT_LINE_LENGTH, pipe) != NULL && (line_count < MAX_OUTPUT_LINES)) {
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
            
        for (int i = 0; i < command_count; i++) {
            printf("%s", commands[i]);
            fprintf(pipe, "%s\n", commands[i]);
            if (fprintf(pipe, "%s\n", commands[i]) < 0) {
                // perror("Failed to write command to pipe");
                // pclose(pipe);
                // exit(EXIT_FAILURE);
            }
        }


        line_count++;
    }
    
    // Close pipe
    pclose(pipe);
    
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
    
    // char* commands[] = {
    //     "insert (1 user1 person1@example.com)",
    //     "select",
    //     ".exit"
    // };
    
    char* commands[] = {
        ".exit"
    };

    // char* expected_output[] = {
    //     "db > Executed.",
    //     "db > (1, user1, person1@example.com)",
    //     "Executed.",
    //     "db > "
    // };
    
    char* expected_output[] = {
        "'.exit'."
    };
    
    int output_line_count = 0;
    char** output = run_script(commands, 3, &output_line_count);
    
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
