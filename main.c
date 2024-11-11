#include <libgen.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define false 0
#define true 1

typedef int bool;

const char *k_see_help = "use --help to view all available commands";
const char *k_etchosts_open = "# ////adhd-cli////\n";
const char *k_etchosts_close = "# \\\\\\\\adhd-cli\\\\\\\\\n";
const char *k_block_line_fmt = "127.0.0.1 %s\n";
const char *k_etchosts_filename = "/etc/hosts";
const char *k_temphosts_filename = "/etc/adhd-hosts";

FILE *etc_hosts = NULL;
char binary_path[PATH_MAX];
char *binary_name = NULL;

void get_binary_path(void)
{
    ssize_t len = readlink("/proc/self/exe", binary_path, sizeof(binary_path) - 1);
    if (len == -1)
    {
        perror("Failed to get binary path");
        exit(1);
    }
    binary_path[len] = '\0';

    char path_copy[PATH_MAX];
    strncpy(path_copy, binary_path, sizeof(path_copy) - 1);

    binary_name = basename(binary_path);
}

long parse_time_with_metric(const char *v)
{
    char *time_metric;
    long time = strtol(v, &time_metric, 10);

    if (strlen(time_metric) != 1)
    {
        printf("argument can be %lds, %ldm or %ldh\n", time, time, time);
        exit(1);
    }

    switch (time_metric[0])
    {
        case 's':
            break;
        case 'm':
            time *= 60;
            break;
        case 'h':
            time *= 3600;
            break;
        default:
            printf("argument can be %lds, %ldm or %ldh\n", time, time, time);
            exit(1);
    }

    return time;
}

void open_etchosts()
{
    if ((etc_hosts = fopen(k_etchosts_filename, "a+")) == NULL)
    {
        printf("error opening %s run with sudo!\n", k_etchosts_filename);
        exit(1);
    }
}

void close_etchosts() { fclose(etc_hosts); }

void comment_cmd()
{
    char line[256];
    bool in_bracket = false;
    FILE *new_etc_hosts = NULL;

    open_etchosts();

    if ((new_etc_hosts = fopen("/etc/adhd-hosts", "w")) == NULL)
    {
        close_etchosts();
        printf("error creating temporary file.");
        exit(1);
    }

    while (fgets(line, sizeof(line), etc_hosts))
    {
        if (in_bracket && strncmp(line, k_etchosts_close, strlen(k_etchosts_close)) == 0)
        {
            in_bracket = false;
        }

        if (in_bracket && line[0] != '#')
        {
            fprintf(new_etc_hosts, "#%s", line);
            continue;
        }

        if (strncmp(line, k_etchosts_open, strlen(k_etchosts_open)) == 0)
        {
            in_bracket = true;
        }

        fputs(line, new_etc_hosts);
    }

    close_etchosts();
    remove(k_etchosts_filename);
    rename(k_temphosts_filename, k_etchosts_filename);
    fclose(new_etc_hosts);
}

void uncomment_cmd()
{
    char line[256];
    bool in_bracket = false;
    FILE *new_etc_hosts = NULL;

    open_etchosts();

    if ((new_etc_hosts = fopen("/etc/adhd-hosts", "w")) == NULL)
    {
        close_etchosts();
        printf("error creating temporary file.");
        exit(1);
    }

    while (fgets(line, sizeof(line), etc_hosts))
    {
        if (in_bracket && strncmp(line, k_etchosts_close, strlen(k_etchosts_close)) == 0)
        {
            in_bracket = false;
        }

        if (in_bracket && line[0] == '#')
        {
            char *line_ptr = line;
            fprintf(new_etc_hosts, "%s", line_ptr + 1);
            continue;
        }

        if (strncmp(line, k_etchosts_open, strlen(k_etchosts_open)) == 0)
        {
            in_bracket = true;
        }

        fputs(line, new_etc_hosts);
    }

    close_etchosts();
    remove(k_etchosts_filename);
    rename(k_temphosts_filename, k_etchosts_filename);
    fclose(new_etc_hosts);
}

void block_cmd(char **domains, int domain_count)
{
    char line[256];
    bool in_bracket = false;
    bool no_brackets = true;
    FILE *new_etc_hosts = NULL;

    uncomment_cmd();

    open_etchosts();

    if ((new_etc_hosts = fopen("/etc/adhd-hosts", "w")) == NULL)
    {
        close_etchosts();
        printf("error creating temporary file.");
        exit(1);
    }

    while (fgets(line, sizeof(line), etc_hosts))
    {
        if (in_bracket && strncmp(line, k_etchosts_close, strlen(k_etchosts_close)) == 0)
        {
            for (int i = 0; i < domain_count; i++)
            {
                fprintf(new_etc_hosts, k_block_line_fmt, domains[i]);
            }
            in_bracket = false;
        }

        if (strncmp(line, k_etchosts_open, strlen(k_etchosts_open)) == 0)
        {
            no_brackets = false;
            in_bracket = true;
        }

        fputs(line, new_etc_hosts);
    }

    if (no_brackets)
    {
        fprintf(new_etc_hosts, "%s", k_etchosts_open);
        for (int i = 0; i < domain_count; i++)
        {
            fprintf(new_etc_hosts, k_block_line_fmt, domains[i]);
        }
        fprintf(new_etc_hosts, "%s", k_etchosts_close);
    }

    close_etchosts();
    remove(k_etchosts_filename);
    rename(k_temphosts_filename, k_etchosts_filename);
    fclose(new_etc_hosts);
}

// INFO: Pass NULL and -1 to delete all domains
void unblock_cmd(char **domains, int domain_count)
{
    char line[256];
    bool in_bracket = false;
    bool delete_all = !domains && domain_count == -1;
    FILE *new_etc_hosts = NULL;

    uncomment_cmd();

    open_etchosts();

    if ((new_etc_hosts = fopen("/etc/adhd-hosts", "w")) == NULL)
    {
        close_etchosts();
        printf("error creating temporary file.");
        exit(1);
    }

    while (fgets(line, sizeof(line), etc_hosts))
    {
        bool put_line = true;

        if (in_bracket && strncmp(line, k_etchosts_close, strlen(k_etchosts_close)) == 0)
        {
            in_bracket = false;
        }

        if (in_bracket)
        {
            if (delete_all)
            {
                put_line = false;
            }
            int line_len = strlen(line);
            // INFO: don't add 1 so \n is not copied
            char *line_copy = malloc(sizeof(char) * line_len);
            strncpy(line_copy, line, line_len - 1);
            char *domain = strtok(line_copy, " ");
            domain = strtok(NULL, " ");
            for (int i = 0; i < domain_count; i++)
            {
                if (strcmp(domain, domains[i]) == 0)
                {
                    put_line = false;
                    break;
                }
            }
            free(line_copy);
        }

        if (strncmp(line, k_etchosts_open, strlen(k_etchosts_open)) == 0)
        {
            in_bracket = true;
        }

        if (put_line)
        {
            fputs(line, new_etc_hosts);
        }
    }

    close_etchosts();
    remove(k_etchosts_filename);
    rename(k_temphosts_filename, k_etchosts_filename);
    fclose(new_etc_hosts);
}

char *get_real_username()
{
    const char *sudo_user = getenv("SUDO_USER");
    if (!sudo_user)
    {
        sudo_user = getenv("LOGNAME");
        if (!sudo_user)
        {
            sudo_user = getenv("USER");
        }
    }
    return strdup(sudo_user);
}

char *get_real_home()
{
    char *sudo_user = get_real_username();

    if (sudo_user)
    {
        struct passwd *pw = getpwnam(sudo_user);
        free(sudo_user);
        if (pw)
        {
            return pw->pw_dir;
        }
    }

    return getenv("HOME");
}

void setup_sudo_rules(const char *real_user)
{
    char sudoers_path[512];
    snprintf(sudoers_path, sizeof(sudoers_path), "/etc/sudoers.d/adhd-cli");

    FILE *sudoers_file = fopen(sudoers_path, "w");
    if (!sudoers_file)
    {
        perror("Failed to create sudoers file");
        return;
    }

    fprintf(sudoers_file,
            "%s ALL=(root) NOPASSWD: %s -c\n"
            "%s ALL=(root) NOPASSWD: %s -uc\n",
            real_user, binary_path, real_user, binary_path);

    fclose(sudoers_file);

    chmod(sudoers_path, 0440);
}

void create_systemd_service(long interval_time, long break_time, int start_hour, int start_min, int end_hour,
                            int end_min)
{
    char *real_home = get_real_home();
    char *real_user = get_real_username();

    setup_sudo_rules(real_user);

    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s/.config/systemd/user", real_home);
    mkdir(dir_path, 0755);

    char service_path[512];
    snprintf(service_path, sizeof(service_path), "%s/.config/systemd/user/adhd-blocker.service", real_home);

    FILE *service_file = fopen(service_path, "w");
    if (!service_file)
    {
        perror("Failed to create service file");
        free(real_user);
        return;
    }

    fprintf(service_file,
            "[Unit]\n"
            "Description=ADHD Domain Blocker Service\n\n"
            "[Service]\n"
            "Type=simple\n"
            "ExecStart=/bin/bash -c 'while true; do sudo %s -uc; sleep %ld; "
            "sudo %s -c; sleep %ld; done'\n"
            "Restart=on-failure\n\n"
            "[Install]\n"
            "WantedBy=default.target\n",
            binary_path, interval_time, binary_path, break_time);

    fclose(service_file);

    char timer_path[512];
    snprintf(timer_path, sizeof(timer_path), "%s/.config/systemd/user/adhd-blocker.timer", real_home);

    FILE *timer_file = fopen(timer_path, "w");
    if (!timer_file)
    {
        perror("Failed to create timer file");
        free(real_user);
        return;
    }

    fprintf(timer_file,
            "[Unit]\n"
            "Description=ADHD Domain Blocker Timer\n\n"
            "[Timer]\n"
            "OnCalendar=*-*-* %02d:%02d:00\n"
            "RemainAfterElapse=no\n\n"
            "[Install]\n"
            "WantedBy=timers.target\n",
            start_hour, start_min);

    fclose(timer_file);

    char chown_cmd[1024];
    snprintf(chown_cmd, sizeof(chown_cmd), "chown -R %s:%s %s/.config/systemd/user/adhd-blocker.*", real_user,
             real_user, real_home);
    system(chown_cmd);

    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    char *enable_service = "";
    if ((local_time->tm_hour > start_hour || (local_time->tm_hour == start_hour && local_time->tm_min >= start_min)) &&
        (local_time->tm_hour < end_hour || (local_time->tm_hour == end_hour && local_time->tm_min < end_min)))
    {
        uncomment_cmd();
        enable_service = " && systemctl --user start adhd-blocker.service";
    }
    else
    {
        comment_cmd();
    }

    char systemd_cmd[1024];
    snprintf(systemd_cmd, sizeof(systemd_cmd),
             "XDG_RUNTIME_DIR=/run/user/$(id -u %s) "
             "su %s -c '"
             "systemctl --user daemon-reload && "
             "systemctl --user enable adhd-blocker.timer && "
             "systemctl --user start adhd-blocker.timer %s"
             "'",
             real_user, real_user, enable_service);

    int result = system(systemd_cmd);
    if (result != 0)
    {
        printf("Failed to start service. Trying alternative method...\n");

        char enable_lingering[256];
        snprintf(enable_lingering, sizeof(enable_lingering), "loginctl enable-linger %s", real_user);
        system(enable_lingering);

        system(systemd_cmd);
    }

    free(real_user);
}

void cleanup_previous_schedule(const char *real_user)
{
    char stop_cmd[512];
    snprintf(stop_cmd, sizeof(stop_cmd),
             "XDG_RUNTIME_DIR=/run/user/$(id -u %s) "
             "su %s -c '"
             "systemctl --user stop adhd-blocker.timer 2>/dev/null; "
             "systemctl --user stop adhd-blocker.service 2>/dev/null; "
             "systemctl --user disable adhd-blocker.timer 2>/dev/null"
             "'",
             real_user, real_user);
    system(stop_cmd);

    char at_cleanup_cmd[512];
    snprintf(at_cleanup_cmd, sizeof(at_cleanup_cmd),
             "su %s -c '"
             "for job in $(at -l | awk \"{print \\$1}\"); do "
             "  if at -c \"$job\" | grep -q \"adhd-blocker\"; then "
             "    atrm \"$job\"; "
             "  fi; "
             "done' 2>/dev/null",
             real_user);
    system(at_cleanup_cmd);
}

void schedule_cmd(char **argv)
{
    char *real_user = get_real_username();
    cleanup_previous_schedule(real_user);

    long interval_time = parse_time_with_metric(argv[0]);
    long break_time = parse_time_with_metric(argv[1]);
    int start_hour, start_min, end_hour, end_min;
    sscanf(argv[2], "%d:%d", &start_hour, &start_min);
    sscanf(argv[3], "%d:%d", &end_hour, &end_min);

    create_systemd_service(interval_time, break_time, start_hour, start_min, end_hour, end_min);

    char stop_cmd[512];
    snprintf(stop_cmd, sizeof(stop_cmd),
             "XDG_RUNTIME_DIR=/run/user/$(id -u %s) "
             "su %s -c '"
             "echo \"XDG_RUNTIME_DIR=/run/user/$(id -u) "
             "systemctl --user stop adhd-blocker.timer && "
             "systemctl --user stop adhd-blocker.service && "
             "sudo %s -c\" | at %02d:%02d"
             "' 2>/dev/null",
             real_user, real_user, binary_path, end_hour, end_min);
    system(stop_cmd);

    printf("Domain blocking scheduled:\n");
    printf("- Start time: %02d:%02d\n", start_hour, start_min);
    printf("- End time: %02d:%02d\n", end_hour, end_min);
    printf("- Work interval: %.2f minutes\n", (float)interval_time / 60);
    printf("- Break interval: %.2f minutes\n", (float)break_time / 60);

    free(real_user);
}

void help_cmd()
{
    printf("adhd-cli - A tool to manage domain blocking for better focus\n\n");

    printf("USAGE:\n");
    printf("  %s <command> [arguments]\n\n", binary_name);

    printf("COMMANDS:\n");
    printf("  -h,  --help       Display this help message\n");

    printf("  -b,  --block      Block specified domains\n");
    printf("                    Usage: -b <domain1> [domain2...]\n");

    printf("  -u,  --unblock    Unblock specified domains\n");
    printf("                    Usage: -u <domain1> [domain2...]\n");

    printf("  -d,  --delete     Remove all domain blocks\n");
    printf("                    Usage: -d\n\n");

    printf("  -s,  --schedule   Schedule domain blocking with intervals\n");
    printf("                    Usage: -s <interval> <break> <start> <end>\n");
    printf("                    Time format: <number>[s|m|h] for interval/break\n");
    printf("                    Time format: HH:MM for start/end\n");

    printf("  -c,  --comment    Comment out domains in hosts file\n");
    printf("                    Usage: -c\n\n");

    printf("  -uc, --uncomment  Uncomment domains in hosts file\n");
    printf("                    Usage: -uc\n\n");

    printf("  -cc, --cancel     Cancel current blocking schedule\n");
    printf("                    Usage: -cc\n\n");

    printf("NOTES:\n");
    printf("  - Must be run with sudo privileges for most operations\n");
    printf("  - Domains are blocked by modifying the /etc/hosts file\n");
    printf("  - Schedule feature uses systemd timers and at jobs\n");
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s -c, --command <arguments>\n%s\n", argv[0], k_see_help);
        return 1;
    }

    get_binary_path();

    printf("%s\n", binary_path);
    printf("%s\n", binary_name);

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
    {
        if (argc != 2)
        {
            printf("Usage: %s -h, --help\n", argv[0]);
            return 1;
        }
        help_cmd();
    }
    else if (strcmp(argv[1], "--block") == 0 || strcmp(argv[1], "-b") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: %s -b, --block <domain1> [domain2...]\n", argv[0]);
            return 1;
        }
        block_cmd(argv + 2, argc - 2);
    }
    else if (strcmp(argv[1], "--unblock") == 0 || strcmp(argv[1], "-u") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: %s -u, --unblock <domain1> [domain2...]\n", argv[0]);
            return 1;
        }
        unblock_cmd(argv + 2, argc - 2);
    }
    else if (strcmp(argv[1], "--delete") == 0 || strcmp(argv[1], "-d") == 0)
    {
        if (argc != 2)
        {
            printf("Usage: %s -d, --delete\n", argv[0]);
            return 1;
        }
        unblock_cmd(NULL, -1);
    }
    else if (strcmp(argv[1], "--schedule") == 0 || strcmp(argv[1], "-s") == 0)
    {
        if (argc != 6)
        {
            printf("Usage: %s -s, --schedule <interval>[s|m|h] <break>[s|m|h] <start> <end>\n", argv[0]);
            return 1;
        }
        schedule_cmd(argv + 2);
    }
    else if (strcmp(argv[1], "--comment") == 0 || strcmp(argv[1], "-c") == 0)
    {
        if (argc != 2)
        {
            printf("Usage: %s -c, --comment\n", argv[0]);
            return 1;
        }
        comment_cmd();
    }
    else if (strcmp(argv[1], "--uncomment") == 0 || strcmp(argv[1], "-uc") == 0)
    {
        if (argc != 2)
        {
            printf("Usage: %s -uc, --uncomment\n", argv[0]);
            return 1;
        }
        uncomment_cmd();
    }
    else if (strcmp(argv[1], "--cancel") == 0 || strcmp(argv[1], "-cc") == 0)
    {
        if (argc != 2)
        {
            printf("Usage: %s -cc, --cancel\n", argv[0]);
            return 1;
        }
        char *real_user = get_real_username();
        cleanup_previous_schedule(real_user);
        free(real_user);
    }
    else
    {
        printf("Unknown command\n%s\n", k_see_help);
    }

    return 0;
}
