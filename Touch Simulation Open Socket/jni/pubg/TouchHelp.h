#include <linux/uinput.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <iconv.h>
#include <mutex>

std::mutex mtx1;

#define sizeoq 11
#define UNGRAB    0
#define GRAB      1

const char *uinput_deivce_path = "/dev/uinput";
float SlideX = 0.0f;
float SlideY = 0.0f;
// 划屏位置
float SlideRanges = 200.0f;
Vec2 aimLocation;
int aimObj = 2;//自瞄方式
//触摸速度
int touchSpeed = 0;
int touchSpeed1 = 0;
int decline = 0;
float screenDistance,WorldDistance;

int x[11],y[11],que[11];
int front=0;
int tid=69;
int fd,fb;

struct AimStruct {
    Vec3 Ox;
    float ScreenDistance;
    float WorldDistance;
    uintptr_t objAddr = 0;
} Aim;

//获取世界距离
float getD3Distance(Vec3 Self, Vec3 Object)
{
	float x, y, z;
	x = Self.X - Object.X;
	y = Self.Y - Object.Y;
	z = Self.Z - Object.Z;
	// 求平方根
	return (float)(sqrt(x * x + y * y + z * z));
}
void enque(int slot)
{
    for(int i=0;i<sizeoq;i++)
    {
        //printf("entered for \n");
        if(que[i]==-1)
        {
            que[i]=slot;
            front++;
            //printf("\n entered,");
            return ;
        }
    }
    //printf("overflow");
    //return -1;
} 
  
//void* perfgest(void* rp);
void execute_sleep(int duration_msec);
double timediff_msec(struct timespec *t1, struct timespec *t2);

int find(int slot)
{
  //  if(tid == 69)
  //      return 8;
    for(int i=0;i<sizeoq;i++)
    {
        if(que[i]==slot)
            return i;
    }
    //printf("not found");
    return -1;
}

void deque(int i)
{
    int j;
    if (front == 0)
    {
        //printf("Underflow \n");
        return ;
    }
    else
    {
        j=find(i);
        //printf("Element deleted from the Queue: %d\n", que[j]);
        que[j] = -1;
        x[j]=-1;
        y[j]=-1;
        front--;
        return;
    }
} 

void syncit()
{
  struct input_event ie;
  for(int i=0;i<sizeoq;i++)
  {
    if(que[i]>=0)
    {
        if(x[i]!=-1&&y[i]!=-1)
        {
      ie.type=EV_ABS;
      ie.code=ABS_MT_POSITION_X;
      ie.value=x[i];
      write(fd, &ie, sizeof(ie));
      ie.type=EV_ABS;
      ie.code=ABS_MT_POSITION_Y;
      ie.value=y[i];
      write(fd, &ie, sizeof(ie));
      ie.type=EV_ABS;
      ie.code=ABS_MT_TRACKING_ID;
      ie.value=que[i];
      write(fd, &ie, sizeof(ie));
        }
      ie.type=EV_SYN;
      ie.code=SYN_MT_REPORT;
      ie.value=0;
      write(fd, &ie, sizeof(ie));
    }
  }
  ie.type=EV_SYN;
  ie.code=SYN_REPORT;
  ie.value=0;
  write(fd, &ie, sizeof(ie));
}

void initarr()
{
  for(int i=0;i<sizeoq;i++)
  {
    x[i]=-1;
    y[i]=-1;
    que[i]=-1;
  }
}

void* TypeA()
{
  
  //struct tas *r=(struct tas*) aa;
  //int fd,fb;
  //fd=fd;
  //fb=fb;
  struct input_event ie;
  struct input_event oy;
  int sv=0, ilsy=0, ptr=-1, j, lsv=-1, ba=0;
  while (read(fb, &ie, sizeof(struct input_event))) 
  {
    if(ie.code==ABS_MT_SLOT)
    {
      sv=ie.value;
      lsv=sv;
    }
    else if(ie.code==SYN_REPORT)
    {
      ilsy=1;
      if(ba!=1)
        ptr++;
      else
        ba=0;
    }
    else if(ilsy==1&&sv>0)
    {
      sv=lsv;
      ilsy=0;
    }
    if(ie.code==ABS_MT_TRACKING_ID)
    {
      if(ie.value == -1)
      {
        deque(sv);
      }
      else
      {
        enque(sv);
      }
    }
    if(ie.code==KEY_APPSELECT)
    {
        //print_event(&ie);
        oy.type=ie.type;
        oy.code=ie.code;
        oy.value=ie.value;
        write(fd, &oy, sizeof(oy));
        ba=1;
    }
    else if(ie.code==KEY_BACK)
    {
        //print_event(&ie);
        oy.type=ie.type;
        oy.code=ie.code;
        oy.value=ie.value;
        write(fd, &oy, sizeof(oy));
        ba=1;
    }
    if(ie.code==ABS_MT_POSITION_X)
    {
      j=find(sv);
      x[j]=ie.value;
    }
    else if(ie.code==ABS_MT_POSITION_Y)
    {
      j=find(sv);
      y[j]=ie.value;
    }
    //printf("\n %d \t %d \n", ptr, front);
    /*if(up==1)
    {
      ptr++;
    }*/
    if(ptr>=front && front!=0)
    {
      ptr=0;
      mtx1.lock();
      syncit();
      mtx1.unlock();
    }
    else if(front==0)
    {
      oy.type=EV_SYN;
      oy.code=SYN_MT_REPORT;
      oy.value=0;
      write(fd, &oy, sizeof(oy));
      oy.type=EV_SYN;
      oy.code=SYN_REPORT;
      oy.value=0;
      write(fd, &oy, sizeof(oy));
      continue;
    }
  }
  return 0;
}

int open_uinput_device(int x,int y)
{
  struct uinput_user_dev ui_dev;
  int uinp_fd = open(uinput_deivce_path, O_WRONLY | O_NONBLOCK);
  if (uinp_fd <= 0) 
  {
          printf("could not open %s, %s\n", uinput_deivce_path, strerror(errno));
          return -1;
  }

  memset(&ui_dev, 0, sizeof(ui_dev));
  strncpy(ui_dev.name, "VirtualTouch",UINPUT_MAX_NAME_SIZE);
  ui_dev.id.bustype = BUS_USB;
  ui_dev.id.vendor = 0x1341;
  ui_dev.id.product = 0x0001;
  ui_dev.id.version = 5;

  ui_dev.absmin[ABS_MT_POSITION_X] = 0;
  ui_dev.absmax[ABS_MT_POSITION_X] = x;
  ui_dev.absmin[ABS_MT_POSITION_Y] = 0;
  ui_dev.absmax[ABS_MT_POSITION_Y] = y;
  ui_dev.absmax[ABS_MT_TRACKING_ID] = 14;
  //enable direct
  ioctl(uinp_fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

  //enable touch event
  ioctl(uinp_fd, UI_SET_EVBIT, EV_ABS);
  //ioctl(uinp_fd, UI_SET_ABSBIT, ABS_X);
  //ioctl(uinp_fd, UI_SET_ABSBIT, ABS_Y);
  ioctl(uinp_fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
  ioctl(uinp_fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
  ioctl(uinp_fd, UI_SET_ABSBIT,ABS_MT_TRACKING_ID);
  ioctl(uinp_fd, UI_SET_EVBIT, EV_SYN);
  ioctl(uinp_fd, UI_SET_EVBIT, EV_KEY);
  ioctl(uinp_fd, UI_SET_KEYBIT, KEY_BACK);
    ioctl(uinp_fd, UI_SET_KEYBIT, KEY_APPSELECT);
  write(uinp_fd, &ui_dev, sizeof(ui_dev));
  if (ioctl(uinp_fd, UI_DEV_CREATE)) 
  {
          printf("Unable to create UINPUT device.\n");
          return -1;
  }
  //printf("\nWorked\n ");

  return uinp_fd;
}

void TouchDown(int tid, int xx,int yy)
{
	int pl;
    enque(tid);
    pl=find(tid);
    x[pl]=xx;
    y[pl]=yy;
    mtx1.lock();
      syncit();
      mtx1.unlock();
}

void TouchMove(int tid,int xx,int yy)
{
    /*
    if (xx >= SlideX + SlideRanges || xx <= SlideX - SlideRanges || yy >= SlideY + SlideRanges
		|| yy <= SlideY - SlideRanges)
	{
		return;
	}
	*/
	int pl;
	pl=find(tid);
	x[pl]=xx;
    y[pl]=yy;
   // if(front==1)
	mtx1.lock();
      syncit();
      mtx1.unlock();
}

void TouchUp(int tid)
{
  deque(tid);
  mtx1.lock();
      syncit();
      mtx1.unlock();
}

int GetEventCount()
{
	DIR *dir = opendir("/dev/input/");
	dirent *ptr = NULL;
	int count = 0;
	while ((ptr = readdir(dir)) != NULL)
	{
		if (strstr(ptr->d_name, "event"))
			count++;
	}
	// printf("count:%d\n",count);
	return count;
}

int GetEventId_2()
{
	int EventCount = GetEventCount();
	int *fdArray = (int *)malloc(EventCount * 4 + 4);
	int result;

	for (int i = 0; i < EventCount; i++)
	{
		char temp[128];
		sprintf(temp, "/dev/input/event%d", i);
		fdArray[i] = open(temp, O_RDWR | O_NONBLOCK);
	}
	//puts("由于您部分原因，设备需要校准，请点击屏幕完成校准");

	int k = 0;
	input_event ev;
	while (1)
	{
		for (int i = 0; i < EventCount; i++)
		{
			memset(&ev, 0, sizeof(ev));
			read(fdArray[i], &ev, sizeof(ev));
			if (ev.type == EV_ABS)
			{
				//printf("id:%d 校准成功\n", i);
				free(fdArray);
				return i;
			}
		}
		usleep(100);
	}
}

int getTouchEventNum()
{
	char name[64];
	char buf[256] = { 0 };
	int fdb = 0;
	int i;
	for (i = 0; i < 32; i++)
	{
		sprintf(name, "/dev/input/event%d", i);
		if ((fdb = open(name, O_RDONLY, 0)) >= 0)
		{
			ioctl(fdb, EVIOCGPHYS(sizeof(buf)), buf);
			if (strstr(buf, "fts") != 0 || strstr(buf, "touch") != 0 || strstr(buf, "Touch"))
			{
				close(fdb);
				return i;
			}
			close(fdb);
		}
	}
	return 0;
}