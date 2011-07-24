#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

#define __set_errno(e) errno = (e)

inline int max(int a,int b) { return a > b ? a : b; };
inline int min(int a,int b) { return a > b ? b : a; };

extern char *strchrnul_darwin (const char *s, int c_in);
char *strndup_darwin(const char *str, size_t len);
ssize_t getdelim_darwin (char **lineptr, size_t *n, int delimiter, FILE *stream);
ssize_t getline_darwin (char **lineptr, size_t *n, FILE *stream);

char *__realpath_darwin (const char *name, char *resolved);
char *canonicalize_file_name_darwin (const char *);

int sscanf_darwin(const char *str, const char *format, ...);

void read_a(const char *str, char **ptr, const char delimiter, int *position);
void read_d(const char *str, int **ptr, const char delimiter, int *position);
void read_x(const char *str, unsigned int **ptr, const char delimiter, int *position);
void read_x_w(const char *str, unsigned int **ptr, int width, int *position);
void read_s_w(const char *str, char *ptr, int width, int *position);
void read_c(const char *str, char *ptr, const char delimiter, int *position);
void read_u(const char *str, unsigned int **ptr, const char delimiter, int *position);
