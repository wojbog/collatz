#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define CHECK(result, textOnFail) \
    if (((long int)result) == -1) \
    {                             \
        perror(textOnFail);       \
        exit(1);                  \
    }

struct myData
{
    int version;
    char text[1020];
};

int main()
{
    int fd = shm_open("/os_cp", O_RDWR | O_CREAT, 0600);
    CHECK(fd, "shm_open failed");
    int r = ftruncate(fd, sizeof(struct myData));
    CHECK(r, "ftruncate failed");
    struct myData *data = mmap(NULL, sizeof(struct myData),
                               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    CHECK(data, "mmap failed");
    close(fd);

    printf("commands:\n"
           "  r           - reads the text\n"
           "  w <text>    - writes new text\n"
           "  q           - quits\n");

    printf("%d",sizeof(struct myData));

    while (1)
    {
        printf("> ");
        fflush(stdout);

        char c, text[1022] = {0};
        scanf("%1021[^\n]", text);
        do
        { // this reads all remaining characters in this line including '\n'
            c = getchar();
            CHECK(c, "getchar EOF'ed");
        } while (c != '\n');

        if (!strlen(text)) // empty line
            continue;

        switch (text[0])
        {
        case 'r':
            printf("version: %d\n   text: %s\n", data->version, data->text);
            break;
        case 'w':
            data->version++;
            strcpy(data->text, text + 2);
            break;
        case 'q':
            munmap(data, sizeof(struct myData));
            exit(0);
            break;
        }
    }
}
