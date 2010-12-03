#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

// Handy macros for sizes
#define B		* (1)
#define KB		* (1024 B)
#define MB		* (1024 KB)
#define GB		* (1024 MB)

// IO Profile
#define FILE_SIZE	(128 MB)
#define CHUNK_SIZE 	(1 MB)
#define FILE_COUNT	4

int main()
{
	int fd[FILE_COUNT], i, j;
	char buf[CHUNK_SIZE] = {'a'};
	char fname[128];

	// Open FILE_COUNT files
	for (i = 0; i < FILE_COUNT; i++) {
		snprintf(fname, 128, "/test/img.%d", i);
		fd[i] = open(fname, O_WRONLY | O_CREAT | O_TRUNC);
		
		if (fd[i] < 0) {
			perror("Error Opening File");
			exit(1);
		}

		// Allocate space for each file, in hopes of getting contiguous sectors
		if (lseek(fd[i], (FILE_SIZE - 1), SEEK_SET) < 0) {
			perror("Error Seeking in File");
			exit(1);
		}
		if (write(fd[i], '\0', 1) != 1) {
			perror ("Error Writing File");
			exit(1);
		}
	}

	// Write FILE_SIZE bytes in CHUNK_SIZE chunks to each file (using a
	// separate process for each file)
	for (i = 0; i < FILE_COUNT; i++) {
		if (fork() == 0) {
			// In the child, write some files in one direction, some in another
			if ((i % 2)) {
				for (j = 0; j < (FILE_SIZE / CHUNK_SIZE); j++) {
					if (write(fd[i], buf, CHUNK_SIZE) != CHUNK_SIZE) {
						perror("Error Writing File");
						exit(1);
					}
					sched_yield();
				}
			}
			else {
				for (j = (FILE_SIZE / CHUNK_SIZE); j >= 0; j--) {
					if (lseek(fd[i], (j * CHUNK_SIZE), SEEK_SET) < 0) {
						perror("Error Seeking in File");
						exit(1);
					}
					if (write(fd[i], buf, CHUNK_SIZE) != CHUNK_SIZE) {
						perror("Error Writing File");
						exit(1);
					}
					sched_yield();
				}
			}
			exit(0);
		}
	}

	// Wait for all children
	for (i = 0; i < FILE_COUNT; i++) {
		waitpid(-1, NULL, 0);
	}

	printf("All Workers Exited\n");
}	
