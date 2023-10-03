
//
// Created by BF on 2021/3/11.
//


#include "struct.h"
#include "TouchHelp.h"

struct Vec3 xyz;
struct D3DMatrix vMat;
	//For Aimbot
	int firing = 0, ads = 0;
	float aimRadius = 200;
	Vec2 pointingAngle;
	uintptr_t yawPitch = 0;
	bool aimbot = true;
	int aimFor = 1;
	bool aimKnoced = false;
	int aimBy = 1;
	int aimWhen = 3;

uintptr_t base;
//骨骼数据
uintptr_t boneAddr;
struct D3DMatrix baseMatrix;
int boneAddrOffset = 0x324;//阵列
int baseMatrixOffset = 0x150;  //基矩阵
int base1Offset = 0x584; //骨骼
//int base1Offset = 0x578; //骨骼
int holdingStateOffset = 0x1B14;//手持武器状态
int weaponOffset = 0x1B24;//武器
int weaponIDOffset = 0x6EC;//武器ID
int weaponAmmoOffset = 0xB5C;//剩余子弹


uintptr_t getMatrix(uintptr_t base);

char *getBone(uintptr_t pBase, struct D3DMatrix viewMatrix);

uintptr_t getEntityAddr(uintptr_t base);

char *getWeaponId(uintptr_t base);

char *getNameByte(uintptr_t address);

PlayerWeapon getPlayerWeapon(uintptr_t base);

PlayerBone getPlayerBone(uintptr_t pBase, struct D3DMatrix viewMatrix);

void p_write(uintptr_t address, void *buffer, int size) {
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = (void *) buffer;
    local[0].iov_len = size;
    remote[0].iov_base = (void *) address;
    remote[0].iov_len = size;

    process_vm_writev(pid, local, 1, remote, 1, 0);
}

int setF(uintptr_t address, float buff) {
    p_write(address, &buff, SIZE);
    return 0;
}

void freezeThread(long int addr1, long int addr2, float x, float y) {
    //setF(addr1, x);
    setF(addr2, y);
    // usleep(10 * 1000); // 10ms
}

Vec2 getPointingAngle(Vec3 camera, Vec3 xyz, float distance) {
    Vec2 PointingAngle;
    float ytime = distance / 88000;

    xyz.Z = xyz.Z + 360 * ytime * ytime;

    float zbcx = xyz.X - camera.X;
    float zbcy = xyz.Y - camera.Y;
    float zbcz = xyz.Z - camera.Z;
    PointingAngle.X = atan2(zbcy, zbcx) * 180 / PI;    // 57.3
    PointingAngle.Y = atan2(zbcz, sqrt(zbcx * zbcx + zbcy * zbcy)) * 180 / PI;

    return PointingAngle;

}

int ActorNum = 0;
uintptr_t *ActorAddress = new uintptr_t[120];

void *Search_PeopleAddress(void *) {
    pMap_A = readmaps();
    pMap_ALL = readmaps_all();
    //人物数组

    uintptr_t Actor = getA(getA(getA(base + 0x6BADC7C) + 0xa8) + 0x270) + 0x100;

    for (;;) {
        usleep(1000);
        int n = 0;
        //数组头部
        uintptr_t TempPointer = getA(Actor);
        int count = getI(Actor + 4);
        // printf("%x %d\n",TempPointer , count);
        for (int s = 0; s < count; s++) {

            usleep(1000);
            uintptr_t TempAddress = getA2(TempPointer + s * 12) + 0x1BC;
            if (!isValid32(TempAddress)) { continue; }
            usleep(1000);
            int temp = getI(TempAddress);
            int value = getI(TempAddress - 204);

            if (value <= 10) { continue; }//22
            if (getF(TempAddress - 0x5C) <= 0) { continue; }
            if (temp == 33577585 || temp == 33577569 || temp == 33577009) {
                ActorAddress[n] = TempAddress;
                n++;
            }
        }
        ActorNum = n;
        printf("%d\n",ActorNum);
    }
}

void *Thread_Address(void *) {
    for (;;) {
        pthread_t gg;
        pthread_create(&gg, NULL, Search_PeopleAddress, NULL);
        pthread_join(gg, NULL);
    }
}

void *Read_Maps(void *) {
    for (;;) {
        pMap_A = readmaps();
        pMap_ALL = readmaps_all();
        sleep(3);
    }
}

void *thread_Maps(void *) {
    for (;;) {
        pthread_t gg;
        pthread_create(&gg, NULL, Read_Maps, NULL);
        pthread_join(gg, NULL);
    }
}


void *AimBotThread() {
    float zm_x, zm_y;
    bool isDown = false;

    double leenx = 0.0f;
    // x轴速度
    double leeny = 0.0f;

    // 滑动距离值，此数值越大，速度越慢，抖动更稳定
    double de = 1.5f; //1.5f
    double tx = SlideX, ty = SlideY;

    double w = 0.0f, h = 0.0f, cmp = 0.0f;
    // 宽度 高度 正切

    //判断触摸是否开启
    while (1) {

       // t.SetTouchFingerNum(FingerMax);
        double ScreenX = height, ScreenY = width;

        // y轴速度
        // 滑动一次值，此数值越大，速度越慢，抖动更稳定
        int speedx = touchSpeed; //250 20
        // 分辨率(竖屏)PS:滑屏用的坐标是竖屏状态下的

        double ScrXH = ScreenX / 2.0f;
        // 一半屏幕X

        double ScrYH = ScreenY / 2.0f;
        // 一半屏幕X

        Vec3 obj = Aim.Ox;
        //向量
      //  Vec3 running;

      //  pvm(Aim.objAddr + vectorOffset, &running, sizeof(running));

        float bulletVelocity = 88000; //getF( getA(getA(uMyObject + weaponOffset) + gunOffset) + bulletVelocityOffset);
        //子弹飞行时间
        float FlyTime = WorldDistance * 100 / bulletVelocity;


        //obj.X += running.X * FlyTime;
        //obj.Y += running.Y * FlyTime;
        //obj.Z += running.Z * FlyTime;
        if(firing)
            obj.Z -= decline;

        Vec3 HeadLocation = World2Screen(vMat, obj);
        //到准星距离
        screenDistance = get2dDistance(width/2, height/2, HeadLocation.X, HeadLocation.Y);
        //世界距离
        WorldDistance = getDistance(xyz, obj);

        bool isAIM = true;
        if (aimObj != 2 || !aimbot ) {
            continue;
        } else {
            // 世界距离
            zm_y = HeadLocation.X;
            // x
            zm_x = (height - HeadLocation.Y);//-   pressure ;
            // y
        }

		if (zm_x == 0 && zm_y == 0)
		{
			continue;
		}
		if (HeadLocation.Z == 1.0f || screenDistance > aimRadius)
            continue;

		if (firing || ads)	// fov发生变化(开镜)
		{
			if (isDown == false)	// 如果没有按下
			{
				TouchDown(tid, tx, ty);
				isDown = true;
				//
				// 赋值为已经按下
			}

			if (zm_x <= 0 || zm_x >= ScreenX || zm_y <= 0 || zm_y >= ScreenY)
				continue;

			if (zm_x < ScrXH)
			{
				w = ScrXH - zm_x;
				// 敌人到准星的横向距离
				leenx = w / speedx;
				if (leenx < 3.0f)
					leenx = screenDistance / (speedx / de);
				if (zm_y < ScrYH)
				{
					h = ScrYH - zm_y;
					// 敌人到准星的纵向距离
					if (w > 1.0f)
					{
						TouchMove(tid, tx, ty -= leenx);
					}
				}
				else
				{
					h = zm_y - ScrYH;
					// 敌人到准星的纵向距离
					if (w > 1.0f)
					{
						TouchMove(tid, tx, ty += leenx);
					}
				}
				cmp = w / h;
				// 正切值
				leeny = h / speedx * cmp;
				if (leeny < 3.0f)
					leeny = screenDistance / (speedx / de);
				if (h > 1.0f)
				{
					TouchMove(tid, tx -= leeny, ty);
				}
			}
			else if (zm_x > ScrXH)
			{
				w = zm_x - ScrXH;
				leenx = w / speedx;
				if (leenx < 3.0f)
					leenx = screenDistance / (speedx / de);
				if (zm_y < ScrYH)
				{
					h = ScrYH - zm_y;
					// 敌人到准星的纵向距离
					if (w > 1.0f)
					{
						TouchMove(tid, tx, ty -= leenx);
					}
				}
				else
				{
					h = zm_y - ScrYH;
					// 敌人到准星的纵向距离
					if (w > 1.0f)
					{
						TouchMove(tid, tx, ty += leenx);
					}
				}
				cmp = w / h;
				// 正切值
				leeny = h / speedx * cmp;
				if (leeny < 3.0f)
					leeny = screenDistance / (speedx / de);
				if (h > 1.0f)
				{
					TouchMove(tid, tx += leeny, ty);
				}
			}

			if (tx >= SlideX + SlideRanges || tx <= SlideX - SlideRanges|| ty >= SlideY + SlideRanges || ty <= SlideY - SlideRanges)
			{
				// 只要滑屏达到了边界，直接还原至中心
				tx = SlideX, ty = SlideY;
				// 恢复变量
				TouchUp(tid);
				usleep(touchSpeed1);
				TouchDown(tid, tx, ty);

			}
		}

		else if ( !(firing || ads))
		{
			if (isDown == true)
			{
				tx = SlideX, ty = SlideY;	// 恢复变
				TouchUp(tid);	// 抬起
				isDown = false;
			}
		}
		usleep(touchSpeed1);
		// 延迟别改，会影响到速度
	}
}


void OoooOoo(){
    // looooooooooooooop
    uintptr_t enAddrPntr;
    float xy0, xy1;

    struct Vec3 screen;
    struct Vec3 exyz;
    int isBack = 0, type = 69;
    int changed = 1;
    int myteamID = 101;
    uintptr_t vMatrix, cLoc;
    vMatrix = getMatrix(base);
    cLoc = vMatrix + 0x290; //xyz
    vMatrix = vMatrix + 0x2A0; //矩阵 650
    uintptr_t fovPntr = vMatrix - 0x100;

    uintptr_t gname_buff[30];
    uintptr_t gname = getA(base + 110706492);


    char cont[0x500];
    char boneData[1024];

    char weaponData[100];


    Request request{};
    Response response{};

    if(ioctl(fb, EVIOCGRAB, GRAB))
        printf("Couldn't grab %s.\n", strerror(errno));
    else
        printf("Grabbed !\n");
    usleep(1000);

    std::thread thr1(AimBotThread);
	std::thread thr2(TypeA);

    while (isBeta || (receive((void *) &request) > 0)) {
        if (!isBeta) {
            height = request.ScreenHeight;
            width = request.ScreenWidth;
        }
        if (request.Mode == InitMode) {
            aimRadius = (float) request.options.aimingRange;
            aimFor = request.options.aimbotmode;
            aimbot = request.options.openState == 0;
            aimWhen = request.options.aimingState;
            aimBy = request.options.priority;
            aimKnoced = request.options.pour;
            aimObj = request.options.aimObj;
            touchSpeed = request.options.touchSpeed;
            touchSpeed1 = request.options.touchSpeed1;
            decline= request.options.decline;
        }

        response.Success = false;
        response.PlayerCount = 0;
        response.VehicleCount = 0;
        response.ItemsCount = 0;
        response.GrenadeCount = 0;
        pvm(cLoc, &xyz, sizeof(xyz));

        pvm(fovPntr, &response.fov, 4);

        pvm(vMatrix, &vMat, sizeof(vMat));


        float nearest = -1.0f;
        firing = 0;
        ads = 0;
        for (int i = 0; i < ActorNum; i++) {

            if (true) {
                //坐标
                if (getF(ActorAddress[i] - 0x5C) <= 0) {
                    continue;
                }
                // 对象指针
                uintptr_t pBase = getA(ActorAddress[i] - 0xF4);
                if (pBase == 0) {
                    continue;
                }
                int ClassID = getI(pBase + 0x10);
                if (ClassID < 1 || ClassID > 2000000) { continue; }
                char ClassName[64] = "";
                uintptr_t FNameEntryArr = getA(gname + (ClassID / 0x4000) * 0x4);
                uintptr_t FNameEntry = getA(FNameEntryArr + (ClassID % 0x4000) * 0x4);
                vm_readv((void*)(FNameEntry + 0x8), ClassName, 64);
                 if (strstr(ClassName, "_Recycled")) { continue; }


                pvm(pBase + 0x89C, healthbuff, sizeof(healthbuff));
                if (healthbuff[1] > 600.0f || healthbuff[1] < 30.0f ||
                    healthbuff[0] > healthbuff[1] || healthbuff[0] < 0.0f)
                    continue;
                PlayerData *data = &response.Players[response.PlayerCount];
                data->Health = healthbuff[0] / healthbuff[1] * 100;

                data->TeamID = getI(pBase + 0x62C);

                //me

                //判断自己地址读取特征
                /* 自身数据 */
                int isMy = getI(ActorAddress[i] + 156);
                int me = getI(pBase + 0x100);
                if (me == 258 || isMy == 94371842) {
                    if (aimbot) {
                        yawPitch = getA(pBase + 0x2F4) + 0x2F4;
                        if (aimWhen == 3 || aimWhen == 2)
                            pvm(pBase + 0Xe0c, &firing, 1);
                        if (aimWhen == 3 || aimWhen == 1)
                            pvm(pBase + 0xa38, &ads, 1);
                    }
                    myteamID = data->TeamID;
                    //自己队伍信息
                    continue;
                } else if (me != 257)
                    continue;
                 if (myteamID == data->TeamID)
                     continue;

                //读取骨骼信息
                boneAddr = getA(pBase + boneAddrOffset);
                baseMatrix = getOMatrix(boneAddr + baseMatrixOffset);
                boneAddr = getA(boneAddr + base1Offset) + 0x30;

                //读取头部信息
                exyz = mat2Cord(getOMatrix(boneAddr + 5 * 48), baseMatrix);
                data->HeadLocation = World2Screen(vMat, exyz);


                data->Distance = getDistance(xyz, exyz);
                if (data->Distance > 600.0f)
                    continue;

                pvm(pBase + 0x644, &data->isBot, sizeof(data->isBot));

                strcpy(data->PlayerNameByte, "66:111:116:");

                if (data->HeadLocation.Z != 1.0f && data->HeadLocation.X < width + 100 &&
                    data->HeadLocation.X > -50) {
                    data->Bone = getPlayerBone(pBase, vMat);
                    if (!data->isBot) {
                        strcpy(data->PlayerNameByte, getNameByte(getA(pBase + 0x5E8)));
                        if (strlen(data->PlayerNameByte) < 4)
                            continue;
                    }
                    data->Weapon = getPlayerWeapon(pBase);
                }


                if (response.PlayerCount >= maxplayerCount) {
                    continue;
                }
                if (data->HeadLocation.Z != 1.0f && (aimKnoced || data->Health > 0) && aimbot) {
                    float centerDist = sqrt((data->HeadLocation.X - width / 2) *
                                            (data->HeadLocation.X - width / 2) +
                                            (data->HeadLocation.Y - height / 2) *
                                            (data->HeadLocation.Y - height / 2));
                    if (centerDist < aimRadius) {
                        if (aimBy != 1)
                            centerDist = data->Distance;
                        if (nearest > centerDist || nearest < 0) {

                            nearest = centerDist;
                            struct D3DMatrix oMatrix;
                            if (aimFor == 1) {
                                oMatrix = getOMatrix(boneAddr + 5 * 48);
                            } else if (aimFor == 2) {
                                oMatrix = getOMatrix(boneAddr + 4 * 48);
                            } else {
                                oMatrix = getOMatrix(boneAddr + 2 * 48);
                            }
                            pointingAngle = getPointingAngle(xyz, mat2Cord(oMatrix, baseMatrix),data->Distance);
                            Aim.Ox = mat2Cord(oMatrix, baseMatrix);
                            Aim.objAddr = pBase;
                            Aim.ScreenDistance = get2dDistance(width/2, height/2, data->HeadLocation.X,data->HeadLocation.Y);
                            Aim.WorldDistance = data->Distance;
                        }
                    }
                }
                response.PlayerCount++;
            }


        }

        if (response.PlayerCount + response.ItemsCount + response.VehicleCount +
            response.GrenadeCount + response.GrenadeCount > 0)
            response.Success = true;


            send((void *) &response, sizeof(response));
            if ((firing || ads) && nearest > 0 && aimObj == 1) {
                thread Modification(freezeThread, yawPitch, yawPitch - 4, pointingAngle.X,pointingAngle.Y);
                Modification.join();



        }

    }
        ioctl(fd, EVIOCGRAB, UNGRAB);
}

uintptr_t getMatrix(uintptr_t base) {
    return getA(getA(base + 115458912) + 0x68);
}

int main() {
    SetValue sv{};

    if (!Create()) {
        perror("Creation Failed");
        return 0;
    }
    if (!Connect()) {
        perror("Connection Failed");
            return 0;
    }

    receive((void *) &sv);

    strcpy(version, "com.tencent.tmgp.pubgmhd");

    pid = getPid(version);
    if (pid < 10) {
        printf("\nGame is not running");
        Close();
        return 0;
    }

    isPremium = true;
    base = getBase();

    pthread_t t1, t2;

    /* 读取Maps文件 */
    pthread_create(&t1, NULL, thread_Maps, NULL);
    /* 遍历人物数组 */
    pthread_create(&t2, NULL, Thread_Address, NULL);

    char l[256];
	sprintf(l, "/dev/input/event%d", GetEventId_2());
	printf("%s", l);
	fb = open(l, O_RDWR);
	if (fb < 0)
	{
		puts("NULL");
		return NULL;
	}
    struct input_absinfo absX;
    struct input_absinfo absY;
    ioctl(fb, EVIOCGABS(ABS_MT_POSITION_X), &absX);
    ioctl(fb, EVIOCGABS(ABS_MT_POSITION_Y), &absY);
    SlideX = (absX.maximum+1) * (830.0f / 1080.0f);
    // 滑屏坐标采用竖屏的坐标，所以这里用的py
    SlideY = (absY.maximum+1) * (1350.0f / 2248.0f);


    initarr();
    fd = open_uinput_device(absX.maximum,absY.maximum);
    assert(fd >= 0);

    OoooOoo();

    return 0;
}


char *getNameByte(uintptr_t address) {
    char static lj[64];
    memset(lj, 0, 64);
    unsigned short int nameI[32];
    pvm(address, nameI, sizeof(nameI));
    char s[10];
    int i;
    for (i = 0; i < 32; i++) {
        if (nameI[i] == 0)
            break;
        sprintf(s, "%d:", nameI[i]);
        strcat(lj, s);
    }
    lj[63] = '\0';

    return lj;
}


PlayerBone getPlayerBone(uintptr_t pBase, struct D3DMatrix viewMatrix) {
    PlayerBone b;
    b.isBone = true;
    struct D3DMatrix oMatrix;
    //uintptr_t boneAddr = getA(pBase + 0x32C);
    //struct D3DMatrix baseMatrix = getOMatrix(boneAddr + 0x150);
    int bones[] = {5, 4, 1, 11, 32, 12, 33, 63, 62, 52, 56, 53, 57, 54, 58};
    // boneAddr = getA(boneAddr + 0x570);
    // neck //脖子0
    oMatrix = getOMatrix(boneAddr + (bones[0]) * 48);
    Vec3 Head = mat2Cord(oMatrix, baseMatrix);
    Head.Z += 7;
    b.neck = World2ScreenMain(viewMatrix, Head);

    // cheast 胸部 1
    oMatrix = getOMatrix(boneAddr + (bones[1]) * 48);
    b.cheast = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // pelvis 2
    oMatrix = getOMatrix(boneAddr + (bones[2]) * 48);
    b.pelvis = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // lSh 3
    oMatrix = getOMatrix(boneAddr + (bones[3]) * 48);
    b.lSh = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // rSh 4
    oMatrix = getOMatrix(boneAddr + (bones[4]) * 48);
    b.rSh = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // lElb 5
    oMatrix = getOMatrix(boneAddr + (bones[5]) * 48);
    b.lElb = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // rElb 6
    oMatrix = getOMatrix(boneAddr + (bones[6]) * 48);
    b.rElb = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // lWr 7
    oMatrix = getOMatrix(boneAddr + (bones[7]) * 48);
    b.lWr = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // rWr 8
    oMatrix = getOMatrix(boneAddr + (bones[8]) * 48);
    b.rWr = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // lTh 9
    oMatrix = getOMatrix(boneAddr + (bones[9]) * 48);
    b.lTh = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // rTh 10
    oMatrix = getOMatrix(boneAddr + (bones[10]) * 48);
    b.rTh = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // lKn 11
    oMatrix = getOMatrix(boneAddr + (bones[11]) * 48);
    b.lKn = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // rKn 12
    oMatrix = getOMatrix(boneAddr + (bones[12]) * 48);
    b.rKn = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // lAn 13
    oMatrix = getOMatrix(boneAddr + (bones[13]) * 48);
    b.lAn = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));
    // rAn 14
    oMatrix = getOMatrix(boneAddr + (bones[14]) * 48);
    b.rAn = World2ScreenMain(viewMatrix, mat2Cord(oMatrix, baseMatrix));

    return b;
}

//武器 子弹数
PlayerWeapon getPlayerWeapon(uintptr_t base) {
    PlayerWeapon p;
    int holdingState = getI(base + holdingStateOffset); //手持状态
    if (holdingState == 1 || holdingState == 2 || holdingState == 3) {
        p.id = getI(getA(base + weaponOffset) + weaponIDOffset); //ID
        p.ammo = getI(getA(base + weaponOffset) + weaponAmmoOffset); //剩余子弹
        p.isWeapon = true;
    }
    return p;
}