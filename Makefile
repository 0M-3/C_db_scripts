CC= gcc
CFLAGS = -Wall -Werror -g

#Paths
SRC_DIR = src/C
BUILD_DIR = build
TEST_DIR = test

#Executables to be generated with the makefile
EXECUTABLE = db
TEST_FILES1 = test
TEST_FILES2 = test_persistent
TEST_FILES3 = test_constants

# Source files (relative to SRC_DIR)
SRC_FILES1 = $(SRC_DIR)/db.c
SRC_TEST_FILES1 = $(TEST_DIR)/test.c
SRC_TEST_FILES2 = $(TEST_DIR)/test_persistent.c
SRC_TEST_FILES3 = $(TEST_DIR)/test_constants.c

# Object files (in BUILD_DIR)

#Default target
all: $(BUILD_DIR)/$(EXECUTABLE) $(BUILD_DIR)/$(TEST_FILES1) $(BUILD_DIR)/$(TEST_FILES2) $(BUILD_DIR)/$(TEST_FILES3)


# Rule to create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)  # Create if not exists

# Rule to create db
$(BUILD_DIR)/$(EXECUTABLE): $(SRC_FILES1) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to create test
$(BUILD_DIR)/$(TEST_FILES1): $(SRC_TEST_FILES1) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^	

# Rule to create test2
$(BUILD_DIR)/$(TEST_FILES2): $(SRC_TEST_FILES2) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^	

# Rule to create test3
$(BUILD_DIR)/$(TEST_FILES3): $(SRC_TEST_FILES3) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^	


# Clean rule
clean:
	rm -rf $(BUILD_DIR)  # Remove the entire build directory


