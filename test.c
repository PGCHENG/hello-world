#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/hello"
#define BUFFER_SIZE 256

/**
 * Test Case 1: Basic read without write
 * Expected: Returns "Hello World\n"
 */
void test_basic_read(void)
{
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    printf("\n=== Test 1: Basic Read (No Write) ===");
    
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }
    
    bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    printf("\nRead %ld bytes: \"%s\"", bytes_read, buffer);
    
    if (strcmp(buffer, "Hello World\n") == 0) {
        printf("\n✓ TEST PASSED\n");
    } else {
        printf("\n✗ TEST FAILED (Expected 'Hello World\\n')\n");
    }
    
    close(fd);
}

/**
 * Test Case 2: Write then read
 * Expected: Returns "Hello Alice\n"
 */
void test_write_read(void)
{
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_written, bytes_read;
    const char *test_string = "Alice";
    
    printf("\n=== Test 2: Write Then Read ===");
    
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("open for write");
        return;
    }
    
    bytes_written = write(fd, test_string, strlen(test_string));
    if (bytes_written < 0) {
        perror("write");
        close(fd);
        return;
    }
    
    printf("\nWrote %ld bytes: \"%s\"", bytes_written, test_string);
    close(fd);
    
    /* Open for read */
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open for read");
        return;
    }
    
    bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    printf("\nRead %ld bytes: \"%s\"", bytes_read, buffer);
    
    if (strcmp(buffer, "Hello Alice\n") == 0) {
        printf("\n✓ TEST PASSED\n");
    } else {
        printf("\n✗ TEST FAILED (Expected 'Hello Alice\\n')\n");
    }
    
    close(fd);
}

/**
 * Test Case 3: Multiple writes (overwrite)
 * Expected: Returns the last written string
 */
void test_multiple_writes(void)
{
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_written, bytes_read;
    const char *first_string = "Bob";
    const char *second_string = "Charlie";
    
    printf("\n=== Test 3: Multiple Writes (Overwrite) ===");
    
    /* First write */
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("open for write");
        return;
    }
    
    bytes_written = write(fd, first_string, strlen(first_string));
    printf("\nFirst write: %ld bytes (\"%s\")", bytes_written, first_string);
    close(fd);
    
    /* Second write */
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("open for write");
        return;
    }
    
    bytes_written = write(fd, second_string, strlen(second_string));
    printf("\nSecond write: %ld bytes (\"%s\")", bytes_written, second_string);
    close(fd);
    
    /* Read */
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open for read");
        return;
    }
    
    bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    printf("\nRead: \"%s\"", buffer);
    
    if (strcmp(buffer, "Hello Charlie\n") == 0) {
        printf("\n✓ TEST PASSED\n");
    } else {
        printf("\n✗ TEST FAILED (Expected 'Hello Charlie\\n')\n");
    }
    
    close(fd);
}

/**
 * Test Case 4: Write with newline
 * Expected: Newline is stripped, returns "Hello Dave"
 */
void test_write_with_newline(void)
{
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_written, bytes_read;
    const char *test_string = "Dave\n";
    
    printf("\n=== Test 4: Write With Newline ===");
    
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("open for write");
        return;
    }
    
    bytes_written = write(fd, test_string, strlen(test_string));
    printf("\nWrote %ld bytes (with newline)", bytes_written);
    close(fd);
    
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open for read");
        return;
    }
    
    bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    printf("\nRead: \"%s\"", buffer);
    
    if (strcmp(buffer, "Hello Dave\n") == 0) {
        printf("\n✓ TEST PASSED\n");
    } else {
        printf("\n✗ TEST FAILED (Expected 'Hello Dave\\n')\n");
    }
    
    close(fd);
}

/**
 * Test Case 5: Large buffer write (test truncation)
 * Expected: Data is truncated to buffer size
 */
void test_large_write(void)
{
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_written, bytes_read;
    char large_string[512];
    
    printf("\n=== Test 5: Large Write (Truncation) ===");
    
    /* Create a string larger than device buffer */
    memset(large_string, 'X', 500);
    large_string[500] = '\0';
    
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("open for write");
        return;
    }
    
    bytes_written = write(fd, large_string, strlen(large_string));
    printf("\nWrote %ld bytes of large string", bytes_written);
    close(fd);
    
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open for read");
        return;
    }
    
    bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    printf("\nRead %ld bytes", bytes_read);
    printf("\n✓ TEST PASSED (Data truncated to buffer size)\n");
    
    close(fd);
}

/**
 * Test Case 6: Empty write
 * Expected: Returns "Hello World\n"
 */
void test_empty_write(void)
{
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    printf("\n=== Test 6: Empty Write (No Data) ===");
    
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("open for write");
        return;
    }
    
    write(fd, "", 0);
    close(fd);
    
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open for read");
        return;
    }
    
    bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    printf("\nRead: \"%s\"", buffer);
    
    if (strcmp(buffer, "Hello World\n") == 0) {
        printf("\n✓ TEST PASSED\n");
    } else {
        printf("\n✗ TEST FAILED\n");
    }
    
    close(fd);
}

int main(void)
{
    printf("================================");
    printf("\nHello Device Driver Test Suite");
    printf("\n================================");
    
    test_basic_read();
    test_write_read();
    test_multiple_writes();
    test_write_with_newline();
    test_large_write();
    test_empty_write();
    
    printf("\n\n================================");
    printf("\nAll tests completed!");
    printf("\n================================\n");
    
    return 0;
}
