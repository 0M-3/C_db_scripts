//#include "../src/C/db.c"
#include <stdio.h>
#include <stdlib.h>

void test_command_read() {
    FILE *fp;
    char buffer[128];
    const char *command = "dir";


    fp = popen(command, "r");
    if (fp==NULL) {
        perror("popen failed. \n");
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, sizeof(buffer), fp)!= NULL) {
        printf("Recieved %s", buffer);
    }

    pclose(fp);
}

// char* run_command(const char* command) {
//     char buffer[128];
//     FILE *fp;

//     fp = popen(command, "r");
//     if (fp==NULL) {
//         perror("popen failed. \n");
//         exit(EXIT_FAILURE);
//     }

//     while (fgets(buffer, sizeof(buffer), fp)!= NULL) {
        
//     }
// }

int main(){
    test_command_read();
    return 0;
}