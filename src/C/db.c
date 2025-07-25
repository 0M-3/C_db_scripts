// #include <errno.h>
// #include <stdio.h>
// #include <stdbool.h>
// #include <stdlib.h>
// #include <string.h>
// #include <stdint.h>
// #include <fcntl.h>
// #include <sys/stat.h>

// #ifdef _WIN32
//     #include <io.h>
//     #include <windows.h>
//     #define open _open
//     #define close _close
//     #define read _read
//     #define write _write
//     #define lseek _lseek
//     #define O_RDWR _O_RDWR
//     #define O_CREAT _O_CREAT
//     #define S_IRUSR _S_IRUSR 
//     #define S_IWUSR _S_IWUSR 
// #else
//     #include <unistd.h>
//     #include <sys/types.h>
// #endif

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>

// This section is the temporary code for storing an in-memory row based database
#define COLUMN_USERNAME_SIZE 12
#define COLUMN_EMAIL_SIZE 255

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE +1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)-> Attribute)

enum {
    ID_SIZE = sizeof(((Row*)0)->id),
    USERNAME_SIZE = sizeof(((Row*)0)->username),
    EMAIL_SIZE = sizeof(((Row*)0)->email),
    ID_OFFSET = 0,
    USERNAME_OFFSET = ID_OFFSET + ID_SIZE,
    EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE,
    ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE,
    PAGE_SIZE = 4096,
    TABLE_MAX_PAGES = 100,
    // Common Node Header Layout
    NODE_TYPE_SIZE = sizeof(uint8_t),
    NODE_TYPE_OFFSET = 0,
    IS_ROOT_SIZE = sizeof(uint8_t),
    IS_ROOT_OFFSET = NODE_TYPE_SIZE,
    PARENT_POINTER_SIZE = sizeof(uint32_t),
    PARENT_POINTER_OFFSET = NODE_TYPE_SIZE+IS_ROOT_SIZE,
    COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE+IS_ROOT_SIZE+PARENT_POINTER_SIZE,

    // Leaf Node Header Layout
    LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t),
    LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE,
    LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE+LEAF_NODE_NUM_CELLS_SIZE,

    // Leaf Node Body Layout
    LEAF_NODE_KEY_SIZE = sizeof(uint32_t),
    LEAF_NODE_KEY_OFFSET = 0,
    LEAF_NODE_VALUE_SIZE = ROW_SIZE,
    LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET+ LEAF_NODE_KEY_SIZE,
    LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE+LEAF_NODE_VALUE_SIZE,
    LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE-LEAF_NODE_HEADER_SIZE,
    LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS/LEAF_NODE_CELL_SIZE,
};


typedef enum {NODE_INTERNAL, NODE_LEAF} NodeType;

uint32_t* leaf_node_num_cells(void* node) {
    return node+LEAF_NODE_NUM_CELLS_OFFSET;
}

uint32_t* leaf_node_cell(void* node, uint32_t cell_num) {
    return node + LEAF_NODE_HEADER_SIZE + cell_num*LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(void* node) {
    *leaf_node_num_cells(node) = 0;
}

void serialize_row(Row* source, void* destination) {
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination) {
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE); 
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

typedef struct
{
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES];
} Pager;


typedef struct {
    uint32_t root_page_num;
    Pager* pager;
} Table;

typedef struct {
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; //
} Cursor;

void print_constants () {
    printf("ROW_SIZE: %d\n", ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

void* get_page(Pager* pager, uint32_t page_num) {
    if (page_num > TABLE_MAX_PAGES) {
        printf("Tried to fetch page numbers out of bounds. %d > %d\n", page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == NULL) {
        //Cache miss. Allocate memory and load from file.
        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->file_length/PAGE_SIZE;

        //We might save a partial page at the end of the file
        if (pager->file_length%PAGE_SIZE) {
            num_pages += 1;
        }

        if (page_num <= num_pages) {
            _lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = _read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read == -1) {
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        pager->pages[page_num] = page;
        if (page_num >= pager->num_pages) {
            pager->num_pages = page_num+1;
        }
    }

    return pager->pages[page_num];

}
void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value) {
    void* node = get_page(cursor->table->pager, cursor->page_num);
    
    uint32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        // Node full
        printf("Need to implement splitting a leaf node.\n");
        exit(EXIT_FAILURE);
    }

    if (cursor->cell_num < num_cells) {
        // Make room  of new cell
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i-1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

void print_leaf_node(void* node) {
    uint32_t num_cells = *leaf_node_num_cells(node);
    printf("leaf (size %d)\n", num_cells);
    for (uint32_t i=0; i < num_cells; i++) {
        uint32_t key = *leaf_node_key(node, i);
        printf(" - %d : %d\n", i, key);
    }
}

Cursor* table_start(Table* table) {
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num=table->root_page_num;
    cursor->cell_num = 0;

    void* root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (num_cells==0);
    return cursor;
}

Cursor* table_end(Table* table) {
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num= table->root_page_num;
    void* root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->cell_num = num_cells;
    cursor->end_of_table = true;

    return cursor;
}


void* cursor_value(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    void* page = get_page(cursor->table->pager, page_num);
    return leaf_node_value(page, cursor->cell_num);
}

void cursor_advance(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    void* node = get_page(cursor->table->pager, page_num);
    cursor->cell_num+=1;
    if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
        cursor->end_of_table = true;
    }
}

void pager_flush(Pager* pager, uint32_t page_num) {
    if (pager->pages[page_num] == NULL) {
        printf("Tried to flush null page\n");
    }

    off_t offset = _lseek(pager->file_descriptor, page_num*PAGE_SIZE, SEEK_SET);

    if (offset==-1) {
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);

    if (bytes_written == -1) {
        printf("Error writting: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}
Pager* pager_open(const char* filename) {
    // int fd = _open(filename,
    //               _O_RDWR | //Read/Write mode
    //               _O_CREAT, //Create files if it does not exist
    //               _S_IREAD | //User write permission
    //               _S_IWRITE //User read permission
    // );
  int fd = open(filename,
                O_RDWR |      // Read/Write mode
                    O_CREAT,  // Create file if it does not exist
                S_IWUSR |     // User write permission
                    S_IRUSR   // User read permission
                );
    if (fd== -1) {
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }

    off_t file_length = _lseek(fd, 0, SEEK_END);

    Pager* pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->num_pages = (file_length/PAGE_SIZE);

    if (file_length % PAGE_SIZE != 0 ) {
        printf("Db files is not a whole number of pages. Corrupt file.\n");
        exit(EXIT_FAILURE);
    }

    for (int32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }

    return pager;
}

Table* db_open(const char* filename) {
    Pager* pager = pager_open(filename);

    Table* table = (Table*)malloc(sizeof(Table));
    table->pager = pager;
    table->root_page_num = 0;

    if (pager->num_pages ==0) {
    // New database file. Initialize page 0 as leaf node.
        void* root_node = get_page(pager, 0);
        initialize_leaf_node(root_node);
    }

    return table;
}

void db_close(Table* table) {
    Pager* pager = table->pager;
    
    for (uint32_t i =0; i< pager->num_pages;i++) {
        if (pager->pages[i] == NULL) {
            continue;
        }
        pager_flush(pager, i);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    // //There may be a partial page remaining at the end. However, this won't be required once a B-Tree structure is implemented for the pager
    // uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    // if (num_additional_rows>0) {
    //     uint32_t page_num  = num_full_pages;
    //     if (pager->pages[page_num] != NULL) {
    //         pager_flush(pager, page_num, num_additional_rows*ROW_SIZE);
    //         free(pager->pages[page_num]);
    //         pager->pages[page_num] = NULL;
    //     }
    // }

    int result = _close(pager->file_descriptor);
    if (result == -1) {
        printf("Error closing the db file.\n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i=0;i<TABLE_MAX_PAGES;i++) {
        void* page = pager->pages[i];
        if (page) {
            free(page);
            pager->pages[i] = NULL;
        }
    }
    free(pager);
    free(table);
}




// void free_table(Table* table) {
//     for (int i=0; table->pages[i]; i++) {
//         free(table->pages[i]);
//     }
//     free(table);
// }

// Permanent code below

typedef enum { EXECUTE_SUCCESS, EXECUTE_TABLE_FULL, EXECUTE_FAILURE } ExecuteResult;

typedef struct
{
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNISED_COMMAND
} MetaCommandResult;

typedef enum { 
        PREPARE_SUCCESS, 
        PREPARE_NEGATIVE_ID,
        PREPARE_STRING_TOO_LONG,
        PREPARE_SYNTAX_ERROR, 
        PREPARE_UNRECOGNISED_STATEMENT,
} PrepareResult;

typedef enum { STATEMENT_INSERT, STATEMENT_SELECT, STATEMENT_FAILED } StatementType;

typedef struct { 
    StatementType type; 
    Row row_to_insert; // only to be used by insert statement, may be temporary
} Statement;

PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement) {
    statement->type = STATEMENT_INSERT;

    strtok(input_buffer->buffer, " ");
    char* id_string = strtok(NULL, " ");
    char* username = strtok(NULL, " ");
    char* email = strtok(NULL, " ");

    if (id_string == NULL || username == NULL || email == NULL) {
        return PREPARE_SYNTAX_ERROR;
    }

    int id = atoi(id_string);
    if (id < 0) {
        return PREPARE_NEGATIVE_ID;
    }
    if (strlen(username) > COLUMN_USERNAME_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }
    if (strlen(email) > COLUMN_EMAIL_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }

    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);

    return PREPARE_SUCCESS;
}


PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
    if (strncmp(input_buffer->buffer, "insert", 6)==0) {
        return prepare_insert(input_buffer, statement);

        // The comment below is the previous implementation of insert statement comprehension.

        /*
        statement->type = STATEMENT_INSERT;
        // Insert function being scanned
        int args_assigned = sscanf(
           input_buffer->buffer, "insert %d %s %s", &(statement->row_to_insert.id),
           statement->row_to_insert.username, statement->row_to_insert.email);
        if (args_assigned<3) {
        return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
        */
    }
    if (strcmp(input_buffer->buffer, "select")==0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNISED_STATEMENT;
}

MetaCommandResult do_meta_command (InputBuffer* input_buffer, Table* table) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        db_close(table);
        exit(EXIT_SUCCESS);
    } else if (strcmp(input_buffer->buffer, ".btree") == 0) {
        printf("Tree:\n");
        print_leaf_node(get_page(table->pager, 0));
        return META_COMMAND_SUCCESS;
    } else if (strcmp(input_buffer->buffer, ".constants") == 0) {
        printf("Constants:\n");
        print_constants();
        return META_COMMAND_SUCCESS;
    } 
    else {
        return META_COMMAND_UNRECOGNISED_COMMAND;
    }
}


InputBuffer* new_input_buffer() {
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
    input_buffer->buffer=NULL;
    input_buffer->buffer_length=0;
    input_buffer->input_length=0;

    return input_buffer;
}

// Temporary Insert and  Select statements

void print_row(Row* row) {
    printf("(%d, %s, %s) \n", row->id, row->username, row->email);
}

ExecuteResult execute_insert (Statement* statement, Table* table){
    void* node = get_page(table->pager, table->root_page_num);
    if ((*leaf_node_num_cells(node) >= LEAF_NODE_MAX_CELLS)) {
        return EXECUTE_TABLE_FULL;
    }

    Row* row_to_insert = &(statement->row_to_insert);
    Cursor* cursor = table_end(table);

    leaf_node_insert(cursor, row_to_insert->id, row_to_insert);

    free(cursor);

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select (Statement* statement, Table* table) {
    Cursor* cursor = table_start(table);
    
    Row row;
    while(!(cursor->end_of_table)) {
        deserialize_row(cursor_value(cursor), &row);
        print_row(&row);
        cursor_advance(cursor);
    }
    free(cursor);

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement (Statement* statement, Table* table) {
    switch(statement->type) {
        case(STATEMENT_INSERT):
            return execute_insert(statement, table);
        case(STATEMENT_SELECT):
            return execute_select(statement, table);
        default:
            return EXECUTE_FAILURE; 
            
    }
}

// End of temporary section 

void print_prompt() {
    printf("db > ");
}

void read_input(InputBuffer* input_buffer) {
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

    if (bytes_read <= 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    //Ignore trailing new line    
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read-1]=0;
}

void close_input_buffer (InputBuffer* input_buffer) {
    free(input_buffer->buffer);
    free(input_buffer);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Must supply a database filename.\n");
        exit(EXIT_FAILURE);
    }

    char* filename = argv[1];
    Table* table = db_open(filename);

    InputBuffer* input_buffer = new_input_buffer();
    while (true) {
        print_prompt();
        read_input(input_buffer);
        // printf("'%s'. \n",input_buffer->buffer);

            if (input_buffer->buffer[0]=='.') {
                switch (do_meta_command(input_buffer, table)) {
                    case(META_COMMAND_SUCCESS):
                        continue;
                    case(META_COMMAND_UNRECOGNISED_COMMAND):
                        printf("Unrecognised command '%s' \n", input_buffer->buffer);
                        continue;
                }
            }
            Statement statement;
            switch (prepare_statement(input_buffer, &statement)) {
                case (PREPARE_SUCCESS):
                    break;
                case (PREPARE_NEGATIVE_ID):
                    printf("ID must be positive.\n");
                    continue;
                case (PREPARE_STRING_TOO_LONG):
                    printf("String is too long.\n");
                    continue;
                case (PREPARE_SYNTAX_ERROR):
                    printf("Syntax error. Could not parse statement.\n");
                    continue;
                case (PREPARE_UNRECOGNISED_STATEMENT):
                    printf("Unrecognised keyword at start of '%s'.\n", input_buffer->buffer);
                    continue;
            }

            //execute_statement(&statement);
            switch (execute_statement(&statement, table))
            {
            case (EXECUTE_SUCCESS):
                printf("Executed. \n");
                break;
            case (EXECUTE_TABLE_FULL):
                printf("Error: Table full. \n");
                break;
            case (EXECUTE_FAILURE):
                printf("Error: Statement failed to generate result. \n");
                break;
            }
        
    }
}