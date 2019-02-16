#include<stdio.h>           // fprintf() printf() stderr getchar() getline() perror()
#include<sys/wait.h>        // waitpid() & associated macros
#include<unistd.h>          // chdir() fork() exec() pid_t
#include<stdlib.h>          // malloc() realloc() free() exit() execvp() EXIT_SUCCESS EXIT_FAILURE
#include<string.h>          // strcmp() strtok()

#define LSH_RL_BUFSIZE      1024
#define LSH_TOK_BUFSIZE     64
#define LSH_TOK_DELIM       " \t\r\n\a"

void lsh_loop(void);
char *lsh_read_line(void);
char **lsh_split_line(char *line);
int lsh_launch(char **args);
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_num_builtins();
int lsh_execute(char **args);

// 内建命令
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

// 内建命令函数的函数指针数组
int (*builtin_func[])(char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

int main(void)
{
    lsh_loop();

    return EXIT_SUCCESS;
}

void lsh_loop(void)
{
    char *line;
    char **args;
    int status;

    do
    {
        printf("> ");
        line = lsh_read_line();             // 读取命令
//        printf("After read line, line = %s\n", line);
        args = lsh_split_line(line);        // 分割命令
//        printf("After split line: \n");
        status = lsh_execute(args);         // 执行命令

        free(line);         // 释放在lsh_read_line函数内部申请的内存
        free(args);         // 释放在lsh_split_line函数内部申请的内存
    }while(status);         // 由状态判断是否继续执行（只有exit命令返回0）
                            // 只有当用户输入exit命令时，status才为0.
}

/***
char *lsh_read_line(void)
{
    char *line = NULL;
    ssize_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}
 */
char *lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;               // LSH_RL_BUFSIZE = 1024
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);          // 申请内存
    int c;

    if(!buffer)             // 如果申请内存失败，打印信息并退出当前程序
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);         // 结束当前程序
    }

    while(1)        // 读取输入
    {
        c = getchar();

        if(c == EOF || c == '\n')               // 当文件或者行结束时，读取完成（EOF是int类型）
        {
            buffer[position] = '\0';            // 如果文件结束，给字符串加上字符串结束标志'\0'
            return buffer;                      // 并返回读取到的行
        }
        else
        {
            buffer[position] = c;               // 如果读取没有结束，继续读取
        }
        position++;

        if(position >= bufsize)                 // 若内存不够，再次申请更大的内存
        {
            bufsize += LSH_RL_BUFSIZE;          // LSH_RL_BUFSIZE = 1024
            buffer = realloc(buffer, bufsize);
            if(!buffer)             // 如果申请失败，打印提示信息并退出当前程序
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;            // LSH_TOK_BUFSIZE = 64
    char **tokens = malloc(bufsize * sizeof(char*));        // 用于分割后的命令的存储
    char *token;                                            // 用于存储分割后命令的每一个部分

    if(!tokens)             // 如果申请内存失败
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);        // strtok()是string.h的函数，按LSH_TOK_DELIM包含的字符分解line字符串
                                                // 每一次执行后返回 指定"要分割的字符串 被分割后 得到的 第一个子字符串"的指针
                                                // 每次执行都只分割出一个符合条件的子串，多次调用直到返回值为NULL表示分割完成。
                                                // 第一次调用后，line 也变成了第一个子串，所以之后的调用需要将line替换成NULL，表示使对上次分解剩下的部分继续分割。
    while(token != NULL)            // 如果上次分割的结果不为NULL，即分割还没有结束
    {
        tokens[position] = token;
        position++;

        if(position >= bufsize)         // position++后判断用于存储子串的内存空间是否足够
        {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));          // 不够就重新分配
            if(!tokens)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, LSH_TOK_DELIM);            // 非第一次调用strtok，所以第一个参数为NULL，真正要分割的原字符串已经存储在一个静态变量中。
    }
    tokens[position] = NULL;        // 分割结束，加上结尾
    return tokens;
}

int lsh_launch(char **args)
{
    pid_t pid;
    int status;

    pid = fork();           // 创建新线程
                            // 子进程返回0
                            // 父进程返回子进程标记（子进程ID）
                            // 创建失败返回-1
                            // fork()之后的代码将会有两个进程运行

    if(pid == 0)        // 子进程运行
    {
        if(execvp(args[0], args) == -1)         // execvp()会从PATH环境变量所指的目录中查找匹配args[0]参数的文件名
                                                // 找到后便执行该文件，并将args参数传递给这个欲执行的文件
                                                // 执行完毕后，如果执行成功没有返回(即直接在此程序中就结束了进程)，如果执行失败返回-1，并将失败原因存储在errno中。
        {
            perror("lsh");              // perror()可以将上一个函数(execvp)执行发生的错误原因打印到stderr中。如果带字符串参数，字符串会先打印，然后再打印错误原因。
        }

//        printf("if run successful, will execute?\n");         // NO

        exit(EXIT_FAILURE);             // 如果失败，结束进程
    }
    else if(pid < 0)    // 创建进程失败
    {
        perror("lsh");      // 以“lsh: 上一个函数(fork)创建进程失败的原因”的格式打印信息
    }
    else                // 父进程运行
    {
        do
        {
            waitpid(pid, &status, WUNTRACED);                   // waitpid()会暂时停止目前进程的执行，直到有信号来到或子进程结束。
                                                                // pid为欲等待的子进程id，子进程的结束状态会由status参数返回。
                                                                // WUNTRACED表示若子进程进入暂停状态，将立即返回，但子进程的结束状态不予理会。

        }while(!WIFEXITED(status) && !WIFSIGNALED(status));     // WIFEXITED(status)表示若status是子进程正常返回的状态，则为真
                                                                // WIFSIGNALED(status)表示若status是子进程异常返回的状态，则为真
                                                                // 所以do循环退出的条件是子进程正常或异常退出。（若子进程不退出，则一直循环）
    }

    return 1;       // 只有父进程会执行(永远返回1)
}

int lsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char*);         // sizeof()是返回字节数的运算符
}

int lsh_cd(char **args)
{
    if(args[1] == NULL)
    {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    }
    else
    {
        if(chdir(args[1]) != 0)         // chdir(args[1])将路径改成args[1]指定的路径。
                                        // 执行成功返回0，失败返回-1，并将错误代码存于errno.
        {
            perror("lsh");
        }
    }
    return 1;
}

int lsh_help(char **args)
{
    int i;
    printf("My shell\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for(i = 0; i < lsh_num_builtins(); i++)         // 显示内建命令
    {
        printf("    %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int lsh_exit(char **args)
{
    return 0;       // 只有在用户输入exit命令退出时才返回0，让shell结束
}

int lsh_execute(char **args)
{
    int i;
    if(args[0] == NULL)         // 针对直接输入回车的响应
    {
        return 1;
    }
    for(i = 0; i < lsh_num_builtins(); i++)             // 找出内建的命令并执行
    {
        if(strcmp(args[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }
    }
    return lsh_launch(args);        // 只是为了调用lsh_launch()，其实一直返回1。因为执行完其他命令后shell并不结束，而是等待执行下一个命令
}

