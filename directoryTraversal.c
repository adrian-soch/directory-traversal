#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

// this function traverses the filesystem like the bash command "ls -lRa" 
int ls_lRa(char *name);

// this function prints directory/file information
int reporter(struct stat sb, char *name);

int main(int argc, char *argv[])
{
    // argv[1] is the path or file name
    // Allowing only a single argument
    if(argc > 2){ 
        printf("Too many arguments supplied\n");
        return 2;
    }
    else if(argc < 2)
    {
        printf("Missing name/path argument\n");
        return 2;
    }

    // calling stat(2) and returning information to the struct sb
    struct stat sb;
    if(stat(argv[1], &sb) < 0){
        perror("main stat");
        return 1;
    }

    // if it is a regular file
    if(S_ISREG(sb.st_mode))
    {
        // printing the file information
        if(reporter(sb, argv[1]) != 0)
        {
            perror("reporter main");
            return 1;
        }
    }
    // if it is a symbolic link
    else if(S_ISLNK(sb.st_mode))
    {
        // symbolic files are not handled by this program
        printf("%s is a symbolic file\n", argv[1]);
    }
    //if it is a directory
    else if(S_ISDIR(sb.st_mode))
    {
        // printing the starting directory name
        printf("%s:\n", argv[1]);

        //calling the fuction on the user inputed directory and handling error
        if(ls_lRa(argv[1]) != 0)
        {
            printf("ls failed main\n");
            return 1;
        }
    }
    return 0;
}

int ls_lRa(char *name)
{
    struct stat sb;
    struct dirent *sDirent;

    char *currName;
    int i = 0;
    char newPath[1024];
    char directs[100][1024];

    //Opening the directory and handling common errors
    DIR *dirp; 
    dirp = opendir(name);
    if(dirp == NULL)
    {
        if(errno == EACCES){
            printf("Permission to %s denied\n", name);
            return 0;}
        if(errno == ENFILE){
            printf("Too many files open currently\n");
            return 0;}
        if(errno == ENOMEM){
            printf("Not enough memory\n");
            return 0;}
        if(errno == ENOENT){
            printf("Directory %s does not exist\n", name);
            return 1;}
    }

    // looping through all entries in the directory
    while( (sDirent = readdir(dirp)) != NULL )
    {
        // creating the next path
        currName = sDirent->d_name;
        snprintf(newPath, sizeof(newPath), "%s/%s", name, currName);

        // gettng the stats
        if(stat(newPath, &sb) < 0){
            perror("stat ls");
            return 1;
        }

        // printing inforation and handling error
        if(reporter(sb, currName) != 0){
            perror("reporter ls");
            return 15;
        }

        // if it is a directory
        if (sDirent->d_type == DT_DIR)
        {
            //Ignoring the current and parent directories
            if ( !strcmp(currName, ".") || !strcmp(currName, ".."))
                continue;

            // Copy paths of the directories to array
            strcpy(directs[i], newPath);
            i++;
        } 
    }

    //Recursively going into each subfolder
    // Doing this after the while loop allows all the files to be in proper order
    for (size_t j = 0; j < i; j++)
    {
        printf("\n");
        printf("%s:\n", directs[j]);
        ls_lRa(directs[j]);         // the recursive call
    }
    
    //Closing Directory and checking for errors
    if(closedir(dirp) < 0){
        perror("closedir");
        return 14;
    }
    return 0;
}

int reporter(struct stat sb, char *name)
{
    struct group *gr;
    struct passwd *pw;
    struct tm date;

    // getting the group name and error handling
    if((gr = getgrgid(sb.st_gid)) == NULL)
    {
        perror("get group id");
        return 11;
    }
    // getting the user name and error handling
    if((pw = getpwuid(sb.st_uid)) == NULL)
    {
        perror("get user id");
        return 12;
    }

    // Code to print the month in string format
    char* month[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Dec"};
    date = *(gmtime(&sb.st_mtime));

    //Reporting details
    printf("%6lo (octal) ",(unsigned long) sb.st_mode);         // mode
    printf(" %ld ", (long) sb.st_nlink);                        // number of links
    printf("UID=%ld / %s ",(long) sb.st_uid, pw->pw_name);      // user id / name
    printf("GID=%ld / %s ",(long) sb.st_gid, gr->gr_name);      // group id / name
    printf("%6lld bytes ",(long long) sb.st_size);              // number of bytes
    printf(" %2lld ",(long long) sb.st_blocks);                 // number of blocks
    printf("%s %2d %2d:%2d ", month[date.tm_mon], date.tm_mday, date.tm_hour,date.tm_min);  // MM dd HH:mm
    printf("%s \n", name);                                      // Name
    return 0;
}