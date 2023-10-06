#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/timer.h>

static struct timer_list proc_monitor_timer;

static void proc_monitor_timer_callback(struct timer_list *timer) {
    struct task_struct *process;

    for_each_process (process) {
        if (process->__state == TASK_RUNNING)
            printk(KERN_INFO "process: %s, state: %u\n", process->comm, process->__state);
    }

    mod_timer(&proc_monitor_timer, jiffies + msecs_to_jiffies(5000));
}

static int __init pn_module_init(void) {
    printk(KERN_INFO "Hello, proc_notify\n");

    timer_setup(&proc_monitor_timer, proc_monitor_timer_callback, 0);
    mod_timer(&proc_monitor_timer, jiffies + msecs_to_jiffies(5000));

    return 0;
}

static void __exit pn_module_exit(void) {
    printk(KERN_INFO "Goodbye, proc_notify\n");
    del_timer(&proc_monitor_timer);
}

module_init(pn_module_init);
module_exit(pn_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Freddy-Wen");
MODULE_DESCRIPTION("A simple module for notify kmod using netlink");
