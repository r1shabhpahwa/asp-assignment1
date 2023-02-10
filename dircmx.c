/* 	
	Author: Rishabh Pahwa (110091353)
	Email: pahwar@uwindsor.ca
	Title: COMP8567: Advanced Systems Programming - Assignment 1
	Submitted to: Dr. Prashanth Ranga
*/

#define _XOPEN_SOURCE 1
#define _XOPEN_SOURCE_EXTENDED 1
#define MAX_EXTENSIONS 6
#define MAX_DEPTH 64
#define MAX_PATH 512
#define DEBUG 0 // will output log messages for each step when set 1

#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

char * src_dir, * dest_dir, * home_dir;
char res_src_dir[MAX_PATH],
   res_dest_dir[MAX_PATH],
   out_dir[2 * MAX_PATH],
   temp_path[MAX_PATH];
int debug_mode = DEBUG;

int mode = 0, num_exts = 0;
char * extensions[MAX_EXTENSIONS];

// Check if file name has a specified extension
int has_extension(const char * file_path) {
   int i;
   const char * ext = strrchr(file_path, '.');

   // no extension found
   if (!ext) return 0;

   // skip the "."
   ext++;
   for (i = 0; i < num_exts; i++) {
      if (strcmp(ext, extensions[i]) == 0) {
         // extension found in the list
         return 1;
      }
   }

   // extension not found in the list
   return 0;
}

// Recursively create directory folders
int create_directory(const char * path) {
   // tmp variable to store the path temporarily
   char tmp[1024];
   char * p = NULL;
   size_t len;

   // copying the path to tmp
   snprintf(tmp, sizeof(tmp), "%s", path);
   len = strlen(tmp);

   // removing the trailing '/' if any
   if (tmp[len - 1] == '/')
      tmp[len - 1] = 0;

   // looping through the string to find the '/'
   for (p = tmp + 1;* p; p++)
      if ( * p == '/') {
         // replacing the '/' with null character *p = 0;
		 * p = 0;
         // creating the directory with read, write, and execute permissions for the owner
		 if (debug_mode) {
            printf("Creating directory: %s\n", tmp);
         }
         mkdir(tmp, S_IRWXU);

         // restoring the '/'
         * p = '/';
      }

   // creating the last directory
   mkdir(tmp, S_IRWXU);

   // returning success
   return 0;
}

// nftw callback function for directory copy/move
int copy_move_callback(const char * fpath,
   const struct stat * sb, int typeflag,
      struct FTW * ftwbuf) {
   int create_dir = 0;

   // Construct destination path
   char dest_path[4096];
   strcpy(dest_path, out_dir);
   strcat(dest_path, fpath + strlen(res_src_dir));

   // Skip files/folders that are not in the extension list if specified
   if (num_exts > 0 && typeflag == FTW_F && !has_extension(fpath)) {
      if (debug_mode) {
         printf("Skipping: %s\n", fpath);
      }
   }

   // Create directory
   else if (typeflag == FTW_D) {
      if (debug_mode) {
         printf("Copying: %s\n", fpath);
         printf("Destination: %s\n", dest_path);
      }

      mkdir(dest_path, S_IRWXU);
   }

   // Copy/move file
   else if (typeflag == FTW_F) {
      if (debug_mode) {
         printf("Copying: %s\n", fpath);
         printf("Destination: %s\n", dest_path);
      }

      FILE * src = fopen(fpath, "r");
      FILE * dest = fopen(dest_path, "w");
      int c;
      while ((c = fgetc(src)) != EOF) {
         fputc(c, dest);
      }

      fclose(src);
      fclose(dest);
   }

   return 0;
}

// nftw callback function for removing directory 
int remove_callback(const char * fpath,
   const struct stat * sb, int typeflag,
      struct FTW * ftwbuf) {
   // Delete enitre source file/directory
   if (debug_mode) {
      if (typeflag == FTW_F) {
         printf("Removing file: %s\n", fpath);
      } else if (typeflag == FTW_DP) {
         printf("Removing directory: %s\n", fpath);
      } else
         printf("Removing: %s\n", fpath);
   }

   remove(fpath);

   return 0;
}

char * resolve_relative(char * path) {

   // resolve relative path "./"
   // tmp variable to store the path temporarily
   char tmp[MAX_PATH];
   char cwd[MAX_PATH];
   char * p = NULL;
   size_t len;

   // copying the path to tmp
   snprintf(tmp, sizeof(tmp), "%s", path);
   len = strlen(tmp);

   if (strncmp(tmp, "./", 2) == 0) {
      getcwd(cwd, sizeof(cwd));
      sprintf(temp_path, "%s%s", cwd, tmp + 1);
   } else if (strncmp(tmp, "../", 3) == 0) {
      getcwd(cwd, sizeof(cwd));
      sprintf(temp_path, "%s%s", dirname(cwd), tmp + 2);

   } else
      strcpy(temp_path, tmp);
   
   // delete trailing "/" in output path
   len = strlen(temp_path);
   if (temp_path[len - 1] == '/')
	   temp_path[len - 1] = '\0';
   
   return temp_path;

}

int main(int argc, char * argv[]) {
   char * base;
   int src_len;

   // Check for the correct number of arguments
   if (argc < 4) {
      printf("Usage: dircmx[source_dir][destination_dir][options] < extension list>\n");
      return 1;
   } else if (argc > 10) {
      printf("Maximum of %d extensions are allowed\n", MAX_EXTENSIONS);
      return 1;
   }
	
   // Check if the specified options is -cp or -mv
   if (strcmp(argv[3], "-cp") == 0) {
      mode = 0;
   } else if (strcmp(argv[3], "-mv") == 0) {
      mode = 1;
   } else {
      printf("Error: Invalid option, use -cp or -mv\n");
      return 1;
   }

   // Check if the extension list is provided and add it to the extensions array
   int i;
   for (i = 4; i < argc; i++) {
      extensions[num_exts++] = argv[i];
   }
   
   // Get home directory 
   home_dir = getenv("HOME");

   // Read source and destination directory from input arguments
   src_dir = argv[1];
   dest_dir = argv[2];

   // resolve relative path for source and destination path
   sprintf(res_src_dir, "%s", resolve_relative(src_dir));
   sprintf(res_dest_dir, "%s", resolve_relative(dest_dir));
   
   src_len = strlen(res_src_dir);

   // Check if the source directory exists
   struct stat src_stat;
   if (stat(res_src_dir, & src_stat) == -1) {
      perror("Error: Source directory does not exist");
      return 1;
   }

   // Check if source and destination are same or if destination is inside source as it will cause infinite loop
   // (also makes no sense in case of -mv as copied directory will get deleted)
   
   if (strncmp(res_src_dir, res_dest_dir, src_len - 1) == 0) {
      printf("Error: Source and destination directory are either same path or destination is inside source directory. \n");
	  printf("Not allowed as this will lead to an infinite loop. \n");
      return 1;
   }
   
   // check if destination

   // Calculate output path
   base = basename(res_src_dir);
   sprintf(out_dir, "%s/%s", res_dest_dir, base);

   // Output Directories
   if (debug_mode) {
      printf("Home directory is: %s\n", home_dir);
      printf("Input Source directory is: %s\n", src_dir);
      printf("Input Destination directory is: %s\n", dest_dir);
      printf("Resolved source directory is: %s\n", res_src_dir);
      printf("Resolved destination directory is: %s\n", res_dest_dir);
      printf("Base source folder is: %s\n", base);
      printf("Output directory is: %s\n", out_dir);
      printf("Option entered is: %s\n\n", argv[3]);
   }

   // Check if source and destination directory are in home directory
   if (strncmp(res_src_dir, home_dir, strlen(home_dir)) ||
      strncmp(res_dest_dir, home_dir, strlen(home_dir))) {
      printf("Error: Both source_dir and destination_dir must belong to the home directory hierarchy.\n");
      return 1;
   }

   // Create output directory if it does not already exist
   create_directory(res_dest_dir);

   
   // Copy or move the directory using nftw
   if (debug_mode) {
      printf("Copy callback: \n");
   }
   
   if (nftw(res_src_dir, copy_move_callback, MAX_DEPTH, FTW_PHYS) != 0) {
      perror("Error: nftw failed");
      return 1;
   }

   // Copy or move the directory using nftw
   // Depth first for deletion as files will be deleted before directory

   if (debug_mode && mode == 1) {
      printf("\nRemove callback:\n");
   }

   if (mode == 1 && nftw(res_src_dir, remove_callback, MAX_DEPTH, FTW_DEPTH | FTW_PHYS) != 0) {
      printf("Error: nftw failed");
      return 1;
   }

   return 0;
}
