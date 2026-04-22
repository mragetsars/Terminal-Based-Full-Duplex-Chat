#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#define SESSION_FILE "ghost_session"
#define MAX_TEXT 1024
#define HIST_FILE "chat_history"

// ساختار پیام برای صف پیام
struct msgbuf {
    long mtype;
    char mtext[MAX_TEXT];
};

int msgid = -1;
int is_user_1 = 0;
long my_send_type, my_recv_type;
pid_t receiver_pid = -1;

// تابع کمکی برای چاپ روی ترمینال با write
void print_str(const char *str) {
    write(1, str, strlen(str));
}

// تابع برای پاک کردن صفحه ترمینال
void clear_screen() {
    write(1, "\033[H\033[J", 6);
}

// تابع هندلر خروج برای پاکسازی منابع
void cleanup_and_exit() {
    if (receiver_pid > 0) {
        kill(receiver_pid, SIGTERM);
        waitpid(receiver_pid, NULL, 0);
    }
    
    if (is_user_1) {
        // حذف فایل سشن و صف پیام فقط توسط نفر اول
        unlink(SESSION_FILE);
        if (msgid != -1) {
            msgctl(msgid, IPC_RMID, NULL);
        }
    }
    
    char hist_filename[32];
    sprintf(hist_filename, "%s_%d.txt", HIST_FILE, is_user_1 ? 1 : 2);
    unlink(hist_filename);
    
    exit(0);
}

// هندلر سیگنال برای مواقعی که کاربر مقابل خارج میشود
void handle_exit_signal(int sig) {
    print_str("\n[SYSTEM] The other user has left the chat\n");
    cleanup_and_exit();
}

// ذخیره پیام در تاریخچه محلی
void log_message(const char* prefix, const char* msg) {
    char hist_filename[32];
    sprintf(hist_filename, "%s_%d.txt", HIST_FILE, is_user_1 ? 1 : 2);
    
    int fd = open(hist_filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd != -1) {
        write(fd, prefix, strlen(prefix));
        write(fd, msg, strlen(msg));
        write(fd, "\n", 1);
        close(fd);
    }
}

// تابع جدید برای حذف پیام از فایل تاریخچه و بازنویسی ترمینال
void remove_message_and_redraw(const char* target_line) {
    char hist_filename[32];
    char temp_filename[32];
    sprintf(hist_filename, "%s_%d.txt", HIST_FILE, is_user_1 ? 1 : 2);
    sprintf(temp_filename, "%s_%d_tmp.txt", HIST_FILE, is_user_1 ? 1 : 2);

    int fd_in = open(hist_filename, O_RDONLY);
    if (fd_in == -1) return;
    
    int fd_out = open(temp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_out == -1) {
        close(fd_in);
        return;
    }

    char line[MAX_TEXT * 2];
    int line_idx = 0;
    char c;
    int removed = 0;

    while (read(fd_in, &c, 1) > 0) {
        if (c == '\n' || line_idx == sizeof(line) - 1) {
            line[line_idx] = '\0';
            if (!removed && strcmp(line, target_line) == 0) {
                removed = 1; 
            } else {
                write(fd_out, line, strlen(line));
                write(fd_out, "\n", 1);
            }
            line_idx = 0;
        } else {
            line[line_idx++] = c;
        }
    }
    close(fd_in);
    close(fd_out);

    unlink(hist_filename);
    link(temp_filename, hist_filename);
    unlink(temp_filename);

    clear_screen();
    print_str("=== Welcome to Ghost Chat ===\n");
    print_str("[SYSTEM] A BURN message just self-destructed!\n");
    
    fd_in = open(hist_filename, O_RDONLY);
    if (fd_in != -1) {
        char buf[1024];
        int n;
        while ((n = read(fd_in, buf, sizeof(buf))) > 0) {
            write(1, buf, n);
        }
        close(fd_in);
    }
    print_str("");
}

// تغییر در تابع هندل کردن پیام زمان‌دار
void handle_burn(const char* prefix, char* msg_content) {
    char temp[MAX_TEXT];
    strcpy(temp, msg_content);
    
    strtok(temp, " ");
    char* sec_str = strtok(NULL, " ");
    if (sec_str == NULL) return;
    
    int seconds = atoi(sec_str);
    
    char target_line[MAX_TEXT * 2];
    strcpy(target_line, prefix);
    strcat(target_line, msg_content);
    
    pid_t burn_pid = fork();
    if (burn_pid == 0) {
        sleep(seconds);
        remove_message_and_redraw(target_line);
        exit(0);
    }
}

int main() {
    signal(SIGINT, cleanup_and_exit);
    signal(SIGUSR1, handle_exit_signal);

    clear_screen();
    print_str("=== Welcome to Ghost Chat ===\n");

    int fd = open(SESSION_FILE, O_CREAT | O_EXCL | O_RDWR, 0666);
    
    if (fd != -1) {
        is_user_1 = 1;
        my_send_type = 1;
        my_recv_type = 2;
        
        msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
        
        char id_str[32];
        int len = sprintf(id_str, "%d", msgid);
        write(fd, id_str, len);
        close(fd);
        
        print_str("[SYSTEM] Waiting for User 2 to join...\n");
    } else {
        is_user_1 = 0;
        my_send_type = 2;
        my_recv_type = 1;
        
        fd = open(SESSION_FILE, O_RDONLY);
        if (fd == -1) {
            print_str("[SYSTEM] Error opening session file.\n");
            return 1;
        }
        
        char id_str[32];
        int n = read(fd, id_str, sizeof(id_str) - 1);
        id_str[n] = '\0';
        close(fd);
        
        msgid = atoi(id_str);
        print_str("[SYSTEM] Connected to User 1's chat!\n");
    }

    receiver_pid = fork();
    
    if (receiver_pid == 0) {
        // *** پردازه فرزند: دریافت کننده پیام (Receiver) ***
        struct msgbuf rcv_msg;
        while (1) {
            if (msgrcv(msgid, &rcv_msg, sizeof(rcv_msg.mtext), my_recv_type, 0) != -1) {
                if (strncmp(rcv_msg.mtext, "SYS_EXIT", 8) == 0) {
                    kill(getppid(), SIGUSR1);
                    exit(0);
                }
                
                // بررسی پیام BURN
                if (strncmp(rcv_msg.mtext, "BURN", 4) == 0) {
                    print_str("[Ghost] ");
                    print_str(rcv_msg.mtext);
                    print_str("\n");
                    
                    log_message("[Ghost] ", rcv_msg.mtext);
                    handle_burn("[Ghost] ", rcv_msg.mtext);
                } else {
                    print_str("[Ghost] ");
                    print_str(rcv_msg.mtext);
                    print_str("\n");
                    log_message("[Ghost] ", rcv_msg.mtext);
                }
            }
        }
    } else {
        // Sender
        struct msgbuf snd_msg;
        snd_msg.mtype = my_send_type;
        
        while (1) {
            print_str("");
            int n = read(0, snd_msg.mtext, MAX_TEXT - 1);
            if (n > 0) {
                if (snd_msg.mtext[n-1] == '\n') {
                    snd_msg.mtext[n-1] = '\0';
                } else {
                    snd_msg.mtext[n] = '\0';
                }
                
                if (strncmp(snd_msg.mtext, "EXIT", 4) == 0) {
                    strcpy(snd_msg.mtext, "SYS_EXIT");
                    msgsnd(msgid, &snd_msg, sizeof(snd_msg.mtext), 0);
                    cleanup_and_exit();
                }
                
                msgsnd(msgid, &snd_msg, sizeof(snd_msg.mtext), 0);
                
                if (strncmp(snd_msg.mtext, "BURN", 4) == 0) {
                    log_message("", snd_msg.mtext);
                    handle_burn("", snd_msg.mtext);
                } else {
                    log_message("", snd_msg.mtext);
                }
            }
        }
    }

    return 0;
}