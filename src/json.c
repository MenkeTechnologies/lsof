/*
 * json.c - JSON output support for lsof
 */

#include "lsof.h"
#include "version.h"

/*
 * json_escape_string() - write a JSON-escaped string to stdout
 */
static void json_escape_string(const char *s) {
    if (!s) {
        fprintf(stdout, "null");
        return;
    }
    fputc('"', stdout);
    for (; *s; s++) {
        switch (*s) {
        case '"':
            fprintf(stdout, "\\\"");
            break;
        case '\\':
            fprintf(stdout, "\\\\");
            break;
        case '\b':
            fprintf(stdout, "\\b");
            break;
        case '\f':
            fprintf(stdout, "\\f");
            break;
        case '\n':
            fprintf(stdout, "\\n");
            break;
        case '\r':
            fprintf(stdout, "\\r");
            break;
        case '\t':
            fprintf(stdout, "\\t");
            break;
        default:
            if ((unsigned char)*s < 0x20) {
                fprintf(stdout, "\\u%04x", (unsigned char)*s);
            } else {
                fputc(*s, stdout);
            }
            break;
        }
    }
    fputc('"', stdout);
}

/*
 * json_begin() - print the opening bracket of the JSON array
 */
void json_begin(void) { fprintf(stdout, "[\n"); }

/*
 * json_end() - print the closing bracket of the JSON array
 */
void json_end(void) { fprintf(stdout, "]\n"); }

/*
 * json_print_proc() - print a process as a JSON object
 */
void json_print_proc(struct lproc *proc, int is_first) {
    struct lfile *lf;
    int first_file;

    if (!proc)
        return;

    /*
     * Process object opening
     */
    if (is_first)
        fprintf(stdout, "  {\n");
    else
        fprintf(stdout, ",\n  {\n");

    /*
     * Process-level fields
     */
    fprintf(stdout, "    \"command\": ");
    json_escape_string(proc->cmd);
    fprintf(stdout, ",\n");

    fprintf(stdout, "    \"pid\": %d,\n", proc->pid);
    fprintf(stdout, "    \"uid\": %lu,\n", (unsigned long)proc->uid);
    fprintf(stdout, "    \"pgid\": %d,\n", proc->pgid);
    fprintf(stdout, "    \"ppid\": %d,\n", proc->ppid);

    /*
     * Files array
     */
    fprintf(stdout, "    \"files\": [\n");

    first_file = 1;
    for (lf = proc->file; lf; lf = lf->next) {
        if (first_file)
            fprintf(stdout, "      {\n");
        else
            fprintf(stdout, ",\n      {\n");

        /*
         * fd
         */
        fprintf(stdout, "        \"fd\": ");
        json_escape_string(lf->fd);

        /*
         * type
         */
        fprintf(stdout, ",\n        \"type\": ");
        json_escape_string(lf->type);

        /*
         * device (major,minor) if defined
         */
        if (lf->dev_def) {
            fprintf(stdout, ",\n        \"device\": \"%u,%u\"",
                    (unsigned)GET_MAJ_DEV(lf->dev),
                    (unsigned)GET_MIN_DEV(lf->dev));
        }

        /*
         * size_off - size or offset
         */
        if (lf->sz_def) {
            fprintf(stdout, ",\n        \"size_off\": %lu",
                    (unsigned long)lf->sz);
        } else if (lf->off_def) {
            fprintf(stdout, ",\n        \"size_off\": %lu",
                    (unsigned long)lf->off);
        }

        /*
         * node (inode number) if inp_ty is 1 (decimal) or 3 (hex)
         */
        if (lf->inp_ty == 1 || lf->inp_ty == 3) {
            fprintf(stdout, ",\n        \"node\": %lu",
                    (unsigned long)lf->inode);
        }

        /*
         * access mode if set
         */
        if (lf->access && lf->access != ' ') {
            fprintf(stdout, ",\n        \"access\": \"%c\"", lf->access);
        }

        /*
         * lock status if set
         */
        if (lf->lock && lf->lock != ' ') {
            fprintf(stdout, ",\n        \"lock\": \"%c\"", lf->lock);
        }

        /*
         * name
         */
        if (lf->name) {
            fprintf(stdout, ",\n        \"name\": ");
            json_escape_string(lf->name);
        }

        fprintf(stdout, "\n      }");
        first_file = 0;
    }

    fprintf(stdout, "\n    ]\n");
    fprintf(stdout, "  }");
}
