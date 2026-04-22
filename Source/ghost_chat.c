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

// پردازش پیام‌های BURN
void handle_burn(char* msg_content) {
    char temp[MAX_TEXT];
    strcpy(temp, msg_content);
    
    // پارس کردن ثانیه و متن (استفاده از توابع رشته مجاز)
    strtok(temp, " "); // پرش از کلمه BURN
    char* sec_str = strtok(NULL, " ");
    char* actual_msg = strtok(NULL, "");
    
    if (sec_str == NULL || actual_msg == NULL) return;
    
    int seconds = atoi(sec_str);
    
    pid_t burn_pid = fork();
    if (burn_pid == 0) {
        // پروسس فرزند برای شمارش معکوس مرگ پیام
        sleep(seconds);
        
        // در یک پیاده سازی کامل تر، در اینجا فایل تاریخچه باز شده، 
        // خط مربوط به پیام حذف میشود و صفحه با clear_screen رفرش میشود.
        // برای سادگی و جلوگیری از اختلال در نوشتن همزمان:
        print_str("\n[SYSTEM] A BURN message just self-destructed!\n");
        
        exit(0);
    }
}

int main() {
    signal(SIGINT, cleanup_and_exit);
    signal(SIGUSR1, handle_exit_signal); // استفاده از SIGUSR1 برای خروج طرف مقابل

    clear_screen();
    print_str("=== Welcome to Ghost Chat ===\n");

    // بررسی اینکه کاربر اول هستیم یا دوم
    int fd = open(SESSION_FILE, O_CREAT | O_EXCL | O_RDWR, 0666);
    
    if (fd != -1) {
        is_user_1 = 1;
        my_send_type = 1;
        my_recv_type = 2;
        
        // ساخت صف پیام جدید
        msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
        
        // نوشتن شناسه صف در فایل
        char id_str[32];
        int len = sprintf(id_str, "%d", msgid);
        write(fd, id_str, len);
        close(fd);
        
        print_str("Waiting for User 2 to join...\n");
    } else {
        is_user_1 = 0;
        my_send_type = 2;
        my_recv_type = 1;
        
        // خواندن شناسه صف از فایل
        fd = open(SESSION_FILE, O_RDONLY);
        if (fd == -1) {
            print_str("Error opening session file.\n");
            return 1;
        }
        
        char id_str[32];
        int n = read(fd, id_str, sizeof(id_str) - 1);
        id_str[n] = '\0';
        close(fd);
        
        msgid = atoi(id_str);
        print_str("Connected to User 1's chat!\n");
    }

    // ایجاد پروسس دوگانه برای چت همزمان
    receiver_pid = fork();
    
    if (receiver_pid == 0) {
        // *** پردازه فرزند: دریافت کننده پیام (Receiver) ***
        struct msgbuf rcv_msg;
        while (1) {
            if (msgrcv(msgid, &rcv_msg, sizeof(rcv_msg.mtext), my_recv_type, 0) != -1) {
                // بررسی پیام خروج
                if (strncmp(rcv_msg.mtext, "SYS_EXIT", 8) == 0) {
                    kill(getppid(), SIGUSR1); // اطلاع به والد برای خروج
                    exit(0);
                }
                
                // بررسی پیام BURN
                if (strncmp(rcv_msg.mtext, "BURN", 4) == 0) {
                    print_str("Stranger (BURN): ");
                    print_str(rcv_msg.mtext);
                    print_str("\n");
                    handle_burn(rcv_msg.mtext);
                } else {
                    print_str("Stranger: ");
                    print_str(rcv_msg.mtext);
                    log_message("Stranger: ", rcv_msg.mtext);
                }
            }
        }
    } else {
        // *** پردازه والد: فرستنده پیام (Sender) ***
        struct msgbuf snd_msg;
        snd_msg.mtype = my_send_type;
        
        while (1) {
            int n = read(0, snd_msg.mtext, MAX_TEXT - 1);
            if (n > 0) {
                snd_msg.mtext[n] = '\0';
                
                // هندل کردن دستور خروج
                if (strncmp(snd_msg.mtext, "EXIT", 4) == 0) {
                    strcpy(snd_msg.mtext, "SYS_EXIT");
                    msgsnd(msgid, &snd_msg, sizeof(snd_msg.mtext), 0);
                    cleanup_and_exit();
                }
                
                msgsnd(msgid, &snd_msg, sizeof(snd_msg.mtext), 0);
                
                if (strncmp(snd_msg.mtext, "BURN", 4) == 0) {
                    handle_burn(snd_msg.mtext);
                } else {
                    log_message("You: ", snd_msg.mtext);
                }
            }
        }
    }

    return 0;
}