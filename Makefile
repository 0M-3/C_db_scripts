CC= gcc
CFLAGS = -Wall -Werror -g

#Paths
SRC_DIR = src/c
BUILD_DIR = build
TEST_DIR = test

#Executables to be generated with the makefile
EXECTUABLE = db 
TEST_FILES1 = test
TEST_FILES2 = test2

# Source files (relative to SRC_DIR)
SRC_FILES1 = $(SRC_DIR)/db.c
SRC_TEST_FILES1 = $(TEST_DIR)/test.c
SRC_TEST_FILES2 = $(TEST_DIR)/test2.c

# Object files (in BUILD_DIR)
OBJ_FILES1 = $(addprefix $(BUILD_DIR)/,$(notdir $(SRC_FILES1:.c=.o)))
OBJ_FILES2 = $(addprefix $(BUILD_DIR)/,$(notdir $(SRC_TEST_FILES1:.c=.o)))
OBJ_FILES3 = $(addprefix $(BUILD_DIR)/,$(notdir $(SRC_TEST_FILES2:.c=.o)))

#Default target
all: $(BUILD_DIR)/$(EXECTUABLES)  $(BUILD_DIR)/$(TEST_FILES1) #$(BUILD_DIR)/$(TEST_FILES2)

# Rule to create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)  # Create if not exists

# Rule to create db
$(BUILD_DIR)/$(EXECUTABLE): $(OBJ_FILES1) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to create test
$(BUILD_DIR)/$(TEST_FILES1): $(OBJ_FILES2) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^	

# # Rule to create test2
# $(BUILD_DIR)/$(TEST_FILES2): $(OBJ_FILES3) | $(BUILD_DIR)
# 	$(CC) $(CFLAGS) -o $@ $^	

# Rule to compile src .c files into .o files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@	

# Rule to compile test c files into .o files
# $(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
# 	$(CC) $(CFLAGS) -c $< -o $@	

# Clean rule
clean:
	rm -rf $(BUILD_DIR)  # Remove the entire build directory


