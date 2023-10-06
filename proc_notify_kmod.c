#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/timer.h>
#include <linux/netlink.h>

#define NETLINK_USER 31
#define NETLINK_GROUP_1 1
#define NETLINK_GROUP_2 2

extern struct net init_net;

static struct timer_list proc_monitor_timer;
static struct sock *nl_sk = NULL;

static void proc_monitor_timer_callback(struct timer_list *timer) {
    struct task_struct *process;

    for_each_process (process) {
        if (process->__state == TASK_RUNNING)
            printk(KERN_INFO "process: %s, state: %u\n", process->comm, process->__state);
    }

    mod_timer(&proc_monitor_timer, jiffies + msecs_to_jiffies(5000));
}

static void send_netlink_message(int group, const char *message) {
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    int msg_size;
    int result;
    
    msg_size = strlen(message) + 1;
    
    skb = nlmsg_new(NLMSG_DEFAULT_SIZE, 0);
    if (!skb) {
        printk(KERN_ALERT "Error creating skb.\n");
        return;
    }
    nlh = nlmsg_put(skb, 0, 0, NLMSG_DONE, msg_size,  0);
    strncpy(nlmsg_data(nlh), message, msg_size);
    
    result = nlmsg_multicast(nl_sk, skb, 0, group, GFP_KERNEL);
    if (result < 0) 
        printk(KERN_ALERT "Error sending to group %d: %d.\n", group, result);
}

static int __init pn_module_init(void) {
    struct netlink_kernel_cfg cfg = {
        .input = NULL, 
    };

    printk(KERN_INFO "Hello, proc_notify\n");

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }

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
