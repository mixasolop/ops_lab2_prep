#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source)                                                            \
  (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),             \
   kill(0, SIGKILL), exit(EXIT_FAILURE))

#define ITER_COUNT 25
volatile sig_atomic_t last_sig;

ssize_t bulk_read(int fd, char *buf, size_t count) {
  ssize_t c;
  ssize_t len = 0;
  do {
    c = TEMP_FAILURE_RETRY(read(fd, buf, count));
    if (c < 0)
      return c;
    if (c == 0)
      return len; // EOF
    buf += c;
    len += c;
    count -= c;
  } while (count > 0);
  return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count) {
  ssize_t c;
  ssize_t len = 0;
  do {
    c = TEMP_FAILURE_RETRY(write(fd, buf, count));
    if (c < 0)
      return c;
    buf += c;
    len += c;
    count -= c;
  } while (count > 0);
  return len;
}

void sethandler(void (*f)(int), int sigNo) {
  struct sigaction act;
  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler = f;
  if (-1 == sigaction(sigNo, &act, NULL))
    ERR("sigaction");
}

void ms_sleep(unsigned int milli) {
  time_t sec = (int)(milli / 1000);
  milli = milli - (sec * 1000);
  struct timespec ts = {0};
  ts.tv_sec = sec;
  ts.tv_nsec = milli * 1000000L;
  if (TEMP_FAILURE_RETRY(nanosleep(&ts, &ts)))
    ERR("nanosleep");
}

void usage(int argc, char *argv[]) {
  printf("%s n\n", argv[0]);
  printf("\t1 <= n <= 4 -- number of moneyboxes\n");
  exit(EXIT_FAILURE);
}

void child_work() {
  printf("[%d] Collection box opened\n", getpid());
  char buf[69];
  snprintf(buf, 67, "skarbona_%d", getpid());
  int fd = open(buf, O_CREAT | O_TRUNC);
  if (fd == -1) {
    ERR("open");
  }
  close(fd);
}

void create_n_collection_boxes(int n, pid_t *arr) {
  for (int i = 0; i < n; i++) {
    pid_t pid = fork();
    arr[i] = pid;
    if (pid == -1) {
      ERR("fork");
    }
    if (pid == 0) {
      child_work();
      exit(EXIT_SUCCESS);
    }
  }
}

void sig_handler(int sig) { last_sig = sig; }

void donor_work() {
  sethandler(sig_handler, SIGUSR1);
  sethandler(sig_handler, SIGUSR2);
  sethandler(sig_handler, SIGPIPE);
  sethandler(sig_handler, SIGINT);

  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigaddset(&mask, SIGUSR2);
  sigaddset(&mask, SIGPIPE);
  sigaddset(&mask, SIGINT);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);
  int box_num;
  while (sigsuspend(&oldmask)) {
    if (last_sig == SIGUSR1) {
      box_num = 0;
      break;
    }
    if (last_sig == SIGUSR2) {
      box_num = 1;
      break;
    }
    if (last_sig == SIGPIPE) {
      box_num = 2;
      break;
    }
    if (last_sig == SIGINT) {
      box_num = 3;
      break;
    }
  }
  printf("[%d] Directed to collection box no %d\n", getpid(), box_num);
}

void create_donors() {
  srand(getpid());
  int sig_arr[4] = {SIGUSR1, SIGUSR2, SIGPIPE, SIGINT};
  for (int i = 0; i < ITER_COUNT; i++) {
    pid_t pid = fork();
    if (pid == -1) {
      ERR("fork");
    }
    if (pid == 0) {
      donor_work();
      exit(EXIT_SUCCESS);
    }
    ms_sleep(100);
    kill(pid, sig_arr[rand() % 4]);
    while (waitpid(pid, NULL, 0) != pid) {
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    usage(argc, argv);
  }
  int num = atoi(argv[1]);
  if (num < 1 || num > 4) {
    usage(argc, argv);
  }
  pid_t children_pid[num];
  create_n_collection_boxes(num, children_pid);
  create_donors();
  while (wait(NULL) > 0) {
  }
  printf("Collection ended!");
}