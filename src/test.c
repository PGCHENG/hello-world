#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/hello"
#define BUFFER_SIZE 256

int main(void)
{
    int fd;
    char write_buffer[BUFFER_SIZE] = "world";
    char read_buffer[BUFFER_SIZE];
    ssize_t bytes_written, bytes_read;

    printf("Opening device %s\n", DEVICE_PATH);

    /* Open the device */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    printf("Device opened successfully (fd=%d)\n", fd);

    /* Write data to the device */
    printf("\nWriting '%s' to device...\n", write_buffer);
    bytes_written = write(fd, write_buffer, strlen(write_buffer));
    if (bytes_written < 0) {
        perror("Failed to write to device");
        close(fd);
        return 1;
    }
    printf("Successfully wrote %ld bytes\n", bytes_written);

    /* Read data from the device */
    printf("\nReading from device...\n");
    memset(read_buffer, 0, BUFFER_SIZE);
    bytes_read = read(fd, read_buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("Failed to read from device");
        close(fd);
        return 1;
    }
    printf("Successfully read %ld bytes\n", bytes_read);
    printf("Read data: %s", read_buffer);

    /* Close the device */
    printf("\nClosing device...\n");
    close(fd);
    printf("Device closed\n");

    return 0;
}
