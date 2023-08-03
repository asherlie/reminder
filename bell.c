#if 0
reminder system
cross platform by using beeps
automatically syncs by local network if two machines
are running the reminder code daemon at once

i can maybe use ICMP echo request packets to get a list of all IPs on the network

notifications will beep n times - n depends on urgency of notification
and will beep every o seconds until acked by user

we'll check periodically for new users of the platform
but once we've found new users, we'll coordinate with them very frequently OR maybe just before alerts, when things happen actually

new user's found, both users exchange all their reminders
then continuously share updates


this mechanism is cool because it also implicitly allows us to set up devices as so called servers
use all the same code but just configure on eexternal IP which can be anywhere and has a lot of uptime


how should reminders be stored on disk?
whatever struct i'm using can just be fwrite()n to a user specified file

there will be a command to output all reminders in multiple formats
will be kind of cool to see a calendar format, a list format
list of only high priority


one of these formats will be the same format that beep reads reminders in
maybe human readable

06-08-1997-10:08:00
!!!
this is a reminder

date
priority
reminder

user will specify a config file
#endif
#include <stdio.h>

struct reminder{
    struct reminder* prev, * next;
    /* n_beeps */
    int priority;
    char* string;
};

/* how should reminders be stored? should it be indexed on priority?
 * should probably just be a linked list ordered by due date actually
 */
struct reminders{
    /*struct reminder* */
};

int main(){
    printf("\a");
    /*printf("%c", '\a');*/
}
