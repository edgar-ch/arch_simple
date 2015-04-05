#define BUF_SIZE 8096

/* Working mode */
enum Mode {CREATE = 1, EXTRACT};
/* Entry type in archive */
enum FType {DIR_T = 1, FILE_T};

/* Header for every file or directory
 * in archive.
 * Archive structure looks like:
 * [ entry header (struct arch_header)]
 * [ nothing if directory, 
 *	 binary stream if file ]
 * .........
 * [ ends with zeroed header]
 * */
struct arch_header {
	char name[PATH_MAX];
	uint64_t size;
	uint8_t type;
	time_t ctime;
};

void create_arch(char *, char *);
void add_to_arch(char *, int);
void write_dir_header(char *, int);
void write_file_to_arch(char *, int);
void extract_arch(char *);
void extract_file_from_arch(struct arch_header *, int);