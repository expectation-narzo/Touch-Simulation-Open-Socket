#include "struct.h"
#include "TouchHelp.h"
#include "Log.h"
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
	int ads2 = 0;
	int firing2 = 0;
	int aimlock = 0;
	

uintptr_t getMatrix(uintptr_t base);
char* getBone(uintptr_t pBase, struct D3DMatrix viewMatrix);
uintptr_t getEntityAddr(uintptr_t base);
uintptr_t getGWorld(uintptr_t base);
char* getWeaponId(uintptr_t base);
char* getNameByte(uintptr_t address);
PlayerWeapon getPlayerWeapon(uintptr_t base);
PlayerBone getPlayerBone(uintptr_t pBase, struct D3DMatrix viewMatrix, MinimalViewInfo POV);
void p_write(uintptr_t address, void* buffer, int size) {
	struct iovec local[1];
	struct iovec remote[1];

	local[0].iov_base = (void*)buffer;
	local[0].iov_len = size;
	remote[0].iov_base = (void*)address;
	remote[0].iov_len = size;

	process_vm_writev(pid, local, 1, remote, 1, 0);
}

struct Actors {
    kaddr Enc_1, Enc_2;
    kaddr Enc_3, Enc_4;
};

struct Chunk {
    uint32_t val_1, val_2, val_3, val_4;
    uint32_t val_5, val_6, val_7, val_8;
};

kaddr DecryptActorsArray(kaddr uLevel, int Actors_Offset, int EncryptedActors_Offset) {
    if (uLevel < 0x10000000)
        return 0;
 
    if (Read<kaddr>(uLevel + Actors_Offset) > 0)
        return uLevel + Actors_Offset;
 
    if (Read<kaddr>(uLevel + EncryptedActors_Offset) > 0) 
        return uLevel + EncryptedActors_Offset;
 
    auto AActors = Read<Actors>(uLevel + EncryptedActors_Offset + 0x10);
 
    if (AActors.Enc_1 > 0) {
        auto Enc = Read<Chunk>(AActors.Enc_1 + 0x80);
        return (((Read<uint8_t>(AActors.Enc_1 + Enc.val_1)
            | (Read<uint8_t>(AActors.Enc_1 + Enc.val_2) << 8))
            | (Read<uint8_t>(AActors.Enc_1 + Enc.val_3) << 0x10)) & 0xFFFFFF
            | ((uint64_t)Read<uint8_t>(AActors.Enc_1 + Enc.val_4) << 0x18)
            | ((uint64_t)Read<uint8_t>(AActors.Enc_1 + Enc.val_5) << 0x20)) & 0xFFFF00FFFFFFFFFF
            | ((uint64_t)Read<uint8_t>(AActors.Enc_1 + Enc.val_6) << 0x28)
            | ((uint64_t)Read<uint8_t>(AActors.Enc_1 + Enc.val_7) << 0x30)
            | ((uint64_t)Read<uint8_t>(AActors.Enc_1 + Enc.val_8) << 0x38);
    }
    else if (AActors.Enc_2 > 0) {
        auto Lost_Actors = Read<kaddr>(AActors.Enc_2);
        if (Lost_Actors > 0) {
            return (uint16_t)(Lost_Actors - 0x400) & 0xFF00
                | (uint8_t)(Lost_Actors - 0x04)
                | (Lost_Actors + 0xFC0000) & 0xFF0000
                | (Lost_Actors - 0x4000000) & 0xFF000000
                | (Lost_Actors + 0xFC00000000) & 0xFF00000000
                | (Lost_Actors + 0xFC0000000000) & 0xFF0000000000
                | (Lost_Actors + 0xFC000000000000) & 0xFF000000000000
                | (Lost_Actors - 0x400000000000000) & 0xFF00000000000000;
        }
    }
    else if (AActors.Enc_3 > 0) {
        auto Lost_Actors = Read<kaddr>(AActors.Enc_3);
        if (Lost_Actors > 0) {
            return (Lost_Actors >> 0x38) | (Lost_Actors << (64 - 0x38));
  }
    }
    else if (AActors.Enc_4 > 0) {
        auto Lost_Actors = Read<kaddr>(AActors.Enc_4);
        if (Lost_Actors > 0) {
            return Lost_Actors ^ 0xCDCD00;
  }
    }
    return 0;
}

Vec2 getPointingAngle(Vec3 camera, Vec3 xyz, float distance)
{
	Vec2 PointingAngle;
	float ytime = distance / 88000;

	xyz.Z = xyz.Z + 360 * ytime * ytime;

	float zbcx = xyz.X - camera.X;
	float zbcy = xyz.Y - camera.Y;
	float zbcz = xyz.Z - camera.Z;
	PointingAngle.X = atan2(zbcy, zbcx) * 180 / PI;	// 57.3
	PointingAngle.Y = atan2(zbcz, sqrt(zbcx * zbcx + zbcy * zbcy)) * 180 / PI;

	return PointingAngle;

}
int setF(uintptr_t address, float buff) {
    p_write(address, &buff, SIZE);
    return 0;
}

void freezeThread(long int addr1, long int addr2, float x, float y) {
    setF(addr1, x);
    setF(addr2, y);
    // usleep(10 * 1000); // 10ms
}

static MinimalViewInfo getPOV(kaddr camera)
{
		return Read<MinimalViewInfo>(camera + 0x430 + 0x10);
}


void *AimBotThread() {
    float zm_x, zm_y;
    bool isDown = false;

    double leenx = 0.0f;
    // x轴速度
    double leeny = 0.0f;

    // 滑动距离值，此数值越大，速度越慢，抖动更稳定
    double de = 1.2f; //1.5f
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
        Vec3 running;

        pvm(getA(Aim.objAddr + 0x1a8) + 0x200, &running, sizeof(running));
        WorldDistance = getDistance(xyz, obj);
        float bulletVelocity = 88000; //getF( getA(getA(uMyObject + weaponOffset) + gunOffset) + bulletVelocityOffset);
        //子弹飞行时间
        float FlyTime = WorldDistance * 100 / bulletVelocity;

        obj.X += running.X * FlyTime;
        obj.Y += running.Y * FlyTime;
        obj.Z += running.Z * FlyTime;
      
        if(firing2)
            obj.Z -= (float)decline / 4.0f * ((WorldDistance) / 30.0f); //2 * decline;

        Vec3 HeadLocation = World2Screen(vMat, obj);
        //到准星距离
        screenDistance = get2dDistance(width/2, height/2, HeadLocation.X, HeadLocation.Y);
        //世界距离
        

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
		if (HeadLocation.Z == 1.0f || screenDistance > aimRadius){
		    	if (isDown == true)
			{
				tx = SlideX, ty = SlideY;	// 恢复变
				TouchUp(tid);	// 抬起
				isDown = false;
				Aim.Ox.X = 0.0f;
				Aim.Ox.Y = 0.0f;
				Aim.objAddr = 0;
			}
            continue;
        }
            
        if(ads2)
		//if (firing || ads)	// fov发生变化(开镜)
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
				if (leenx < 2.0f)
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
				if (leeny < 2.0f)
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
				if (leenx < 2.0f)
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
				if (leeny < 2.0f)
					leeny = screenDistance / (speedx / de);
				if (h > 1.0f)
				{
					TouchMove(tid, tx += leeny, ty);
				}
			}
			/*
			if (tx >= SlideX + SlideRanges || tx <= SlideX - SlideRanges|| ty >= SlideY + SlideRanges || ty <= SlideY - SlideRanges)
			{
				// 只要滑屏达到了边界，直接还原至中心
				tx = SlideX, ty = SlideY;
				// 恢复变量
				TouchUp(tid);
				usleep(touchSpeed1);
				TouchDown(tid, tx, ty);

			}
			*/
		}
		
		else if(!ads2)
		//else if ( !(firing || ads))
		{
			if (isDown == true)
			{
				tx = SlideX, ty = SlideY;	// 恢复变
				TouchUp(tid);	// 抬起
				isDown = false;
				Aim.Ox.X = 0.0f;
				Aim.Ox.Y = 0.0f;
				Aim.objAddr = 0;
			}
		}
		usleep(touchSpeed1);
		// 延迟别改，会影响到速度
	}
}


int main()
{
	SetValue sv{};
	//isBeta = 1;
	char sText[400];
	if (!isBeta)
	{
		if (!Create())
		{
			//if (isBeta == 1)
				perror("Creation Failed");
			return 0;
		}
		if (!Connect())
		{
			//if (isBeta == 1)
				perror("Connection Failed");
			return 0;
		}
		int no;


		receive((void*)&sv);
		no = sv.mode;


		if (no == 1)
			strcpy(version, "com.tencent.ig");
		else if (no == 2)
			strcpy(version, "com.pubg.krmobile");
		else if (no == 3)
			strcpy(version, "com.vng.pubgmobile");
		else
			strcpy(version, "com.rekoo.pubgm");

	}


	// betaend

//strcpy(version,"com.vng.pubgmobile");
	pid = getPid(version);
	if (pid < 10)
	{
		printf("\nGame is not running");
		Close();
		return 0;
	}
	if (isBeta == 1)
		printf("\n Game Pid: %d", pid);

	isPremium = true;
	uintptr_t base = getBase();
	if (isBeta == 1)
		printf("\n Base: %lX\n", base);

	uintptr_t vMatrix = getMatrix(base);
	if (!vMatrix)
	{
		if (isBeta == 1)
			puts("\nMatrix Not Found");
		return 0;
	}
	if (isBeta == 1)
		printf("\nvMatrix: %lX", vMatrix);



	// looooooooooooooop
	uintptr_t enAddrPntr;
	float xy0, xy1;

	struct Vec3 screen;
	struct Vec3 exyz;
	struct Vec3 exyzAim;
	uintptr_t targetAim = 0;
	int isBack = 0, type = 69;
	int changed = 1;
	int myteamID = 101;
	uintptr_t cLoc = vMatrix + 0x750;
	uintptr_t fovPntr = vMatrix + 0x660;
	vMatrix = vMatrix + 0x590;
	char loaded[0x4000], loadedpn[20];
	char name[100];

	uintptr_t gname_buff[30];
	uintptr_t gname = getA(getA(base + 0xb79a750) + 0x110);
	if (strstr(version, "com.tencent.ig"))
		gname = getA(getA(base + 0xb79a750) + 0x110);
	pvm(gname, &gname_buff, sizeof(gname_buff));
	char cont[0x500];
	char boneData[1024];

	char weaponData[100];

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

    if(ioctl(fb, EVIOCGRAB, GRAB))
        printf("Couldn't grab %s.\n", strerror(errno));
    else
        printf("Grabbed !\n");
    usleep(1000);



	std::thread thr1(AimBotThread);
	std::thread thr2(TypeA);

	Request request{};
	Response response{};

	while (isBeta ||  (receive((void*)&request) > 0)) {
		if (!isBeta) {
			height = request.ScreenHeight;
			width = request.ScreenWidth;
		}
		if (request.Mode == InitMode) {
			aimRadius = (float)request.options.aimingRange;
			aimFor = request.options.aimbotmode;
			aimbot = request.options.openState==0;
			aimWhen = request.options.aimingState;
			aimBy = request.options.priority;
			aimKnoced = request.options.pour;
            aimObj = request.options.aimObj;
            touchSpeed = request.options.touchSpeed;
            touchSpeed1 = request.options.touchSpeed1;
            decline= request.options.decline;
            aimlock= 1; // request.options.aimlock;
            
            LOGE("aimRadius %f", aimRadius);
            LOGE("aimlock %d", request.options.aimlock);
            LOGE("aimWhen %d", request.options.aimingState);
            LOGE("speed1 %d", touchSpeed);
            
		}

		response.Success = false;
		response.PlayerCount = 0;
		response.VehicleCount = 0;
		response.ItemsCount = 0;
		response.GrenadeCount = 0;
		pvm(cLoc, &xyz, sizeof(xyz));
		if ((xyz.Z == 88.441124f || xyz.X == 0 || xyz.Z == 5278.43f || xyz.Z == 88.440918f) && !isBeta)
		{
			changed = 1;
			send((void*)&response, sizeof(response));
			continue;
		}
		pvm(fovPntr, &response.fov, 4);


		pvm(vMatrix, &vMat, sizeof(vMat));
		if (isBeta)
			printf("\nvMatChk: %0.1f | FOV: %0.2f | XYZ: %f %f %f", vMat._43, response.fov, xyz.X, xyz.Y, xyz.Z);
		// end


		// enList
		if (changed == 1){
			enAddrPntr = getEntityAddr(base);
			changed = 0;
		}
		Ulevel ulevel;
		
		kaddr actorArr = DecryptActorsArray(enAddrPntr - 0xa0, 0xA0, 0x448);
		ulevel.addr = getA(actorArr);
		ulevel.size = getI(actorArr + 8);
		//pvm(enAddrPntr, &ulevel, sizeof(ulevel));
		if (ulevel.size < 1  || ulevel.size > 0x1000 || !isValid64(ulevel.addr)){
			if (isBeta)
				puts("\nWrong Entity Address");
			changed = 1;
			if (!isBeta) {
				send((void*)&response, sizeof(response));
				continue;
			}
		}
		if (isBeta)
			printf("\nEntity Address: %lX | Size: %d", enAddrPntr, ulevel.size);

		strcpy(loaded, "");
		float nearest = -1.0f;
		firing = 0;
		ads = 0;
		
		kaddr gWorld = getGWorld(base);
		if(!gWorld) continue;
		
		MinimalViewInfo POV = MinimalViewInfo();
		kaddr PlayerController = getA(getA(getA(gWorld + 0x38) + 0x78) + 0x30);
		if(PlayerController) {
		    kaddr camm = getA(PlayerController + 0x458);
		    POV = getPOV(camm);
		}
		
		for (int i = 0; i < ulevel.size; i++) {
			uintptr_t pBase = getA(ulevel.addr + i * SIZE);
			if (!isValid64(pBase))
				continue;
			if (getI(pBase + SIZE) != 8)
				continue;
			int ids = getI(pBase + 8 + 2 * SIZE);
			int page = ids / 0x4000;
			int index = ids % 0x4000;
			if (page < 1 || page>30)
				continue;
			if (gname_buff[page] == 0)
				gname_buff[page] = getA(gname + page * SIZE);
			strcpy(name, getText(getA(gname_buff[page] + index * SIZE)));
			if (strlen(name) < 5)
				continue;

			if (strstr(name, "BP_PlayerPawn")) {//Player
				sprintf(loadedpn, "%lx,", pBase);
				if (strstr(loaded, loadedpn))
					continue;
				strcat(loaded, loadedpn);
				
				/*
				int oType = getI(pBase + 0x88);
				if (oType != 0x3ec2a00 && oType != 0x3ec2800)
					continue;

				if (getI(pBase + 0xb84))
					continue;
				*/
				
				pvm(pBase + 0xcd0, healthbuff, sizeof(healthbuff));
				if (healthbuff[1] > 200.0f || healthbuff[1] < 50.0f || healthbuff[0]>healthbuff[1] || healthbuff[0] < 0.0f)
					continue;
				PlayerData* data = &response.Players[response.PlayerCount];
				data->Health = healthbuff[0] / healthbuff[1] * 100;

				data->TeamID = getI(pBase + 0x8c8);

				//me
				uintptr_t me = getI(pBase + 0x148);
				if (me == 258) {
					if (isBeta)
						printf("\nMe(%d): %lX ", data->TeamID, pBase);
					//if (aimbot) {
						yawPitch = getA(pBase + 0x3f8) + 0x3f0;
						if(aimWhen==3 || aimWhen==2)
							pvm(pBase + 0x1508, &firing, 1);
						if (aimWhen == 3 || aimWhen == 1)
							pvm(pBase + 0xf59, &ads, 1);
					//}
					myteamID = data->TeamID;
					continue;
				}
				else if (me != 257)
					continue;


				if (data->TeamID == myteamID && myteamID<=100)
					continue;


				pvm(getA(pBase + 0x6228) + 0x1b0, &exyz, sizeof(exyz));

				data->HeadLocation = WorldToScreen(exyz, POV, width, height); //World2Screen(vMat, exyz);

				data->Distance = getDistance(xyz, exyz);
				if (data->Distance > 600.0f)
					continue;
				pvm(pBase + 0x958, &data->isBot, sizeof(data->isBot));

				strcpy(data->PlayerNameByte, "66:111:116:");

				if (data->HeadLocation.Z != 1.0f && data->HeadLocation.X < width + 100 && data->HeadLocation.X > -50) {
						data->Bone = getPlayerBone(pBase, vMat, POV);
					if (!data->isBot) {

						strcpy(data->PlayerNameByte, getNameByte(getA(pBase + 0x880)));
						if (strlen(data->PlayerNameByte) < 4)
							continue;
					}
					data->Weapon = getPlayerWeapon(pBase);
				}


				if (response.PlayerCount >= maxplayerCount) {
					continue;
				}
				if (data->HeadLocation.Z != 1.0f && (aimKnoced || data->Health > 0) && aimbot) {
					float centerDist = sqrt((data->HeadLocation.X - width / 2) * (data->HeadLocation.X - width / 2) + (data->HeadLocation.Y - height / 2) * (data->HeadLocation.Y - height / 2));
					if (centerDist < aimRadius) {
						if (aimBy != 1)
							centerDist = data->Distance;
						if (nearest > centerDist || nearest < 0) {
						
						    targetAim = pBase;
						    
							nearest = centerDist;
							if (aimFor == 1){
                                exyzAim = exyz;
                                targetAim = pBase;
                                Aim.ScreenDistance = get2dDistance(width/2, height/2, data->HeadLocation.X,data->HeadLocation.Y);
                                Aim.WorldDistance = data->Distance;
							    pointingAngle = getPointingAngle(xyz, exyz, data->Distance);
							}

							else if (aimFor == 2) {

								uintptr_t boneAddr = getA(pBase + 0x420);
								struct D3DMatrix baseMatrix = getOMatrix(boneAddr + 0x1a0);
								boneAddr = getA(boneAddr + 0x7a8);
								struct D3DMatrix oMatrix = getOMatrix(boneAddr + 4 * 48);

								pointingAngle = getPointingAngle(xyz, mat2Cord(oMatrix, baseMatrix), data->Distance);
                                exyzAim = mat2Cord(oMatrix, baseMatrix);
                                targetAim = pBase;
                                Aim.ScreenDistance = get2dDistance(width/2, height/2, data->HeadLocation.X,data->HeadLocation.Y);
                                Aim.WorldDistance = data->Distance;
							}
							else {
								uintptr_t boneAddr = getA(pBase + 0x420);
								struct D3DMatrix baseMatrix = getOMatrix(boneAddr + 0x1a0);
								boneAddr = getA(boneAddr + 0x7a8);
								struct D3DMatrix oMatrix = getOMatrix(boneAddr + 2 * 48);
								pointingAngle = getPointingAngle(xyz, mat2Cord(oMatrix, baseMatrix), data->Distance);
                                exyzAim = mat2Cord(oMatrix, baseMatrix);
                                targetAim = pBase;
                                Aim.ScreenDistance = get2dDistance(width/2, height/2, data->HeadLocation.X,data->HeadLocation.Y);
                                Aim.WorldDistance = data->Distance;
							}


						}


					}

				}
				response.PlayerCount++;

				if (isBeta)
					printf("\nE | %lX > TI:%d | H:%0.1f | XY: %0.1f %0.1f | %d", pBase, data->TeamID, data->Health, data->HeadLocation.X, data->HeadLocation.Y, data->isBot);

			}
			else if (strstr(name, "VH") || (strstr(name, "PickUp_") && !strstr(name, "BP")) || strstr(name, "Rony") || strstr(name, "Mirado") || strstr(name, "LadaNiva") || strstr(name, "AquaRail")) {//Vehicle
				if (!isPremium)
					continue;
				VehicleData* data = &response.Vehicles[response.VehicleCount];
				pvm(getA(pBase + 0x1a8) + 0x1b0, &exyz, sizeof(exyz));

				data->Location = WorldToScreen(exyz, POV, width, height);//World2Screen(vMat, exyz);
				if (data->Location.Z == 1.0f || data->Location.X > width + 200 || data->Location.X < -200)
					continue;
				data->Distance = getDistance(xyz, exyz);
				strcpy(data->VehicleName, name);
				if (response.VehicleCount >= maxvehicleCount) {
					continue;
				}
				response.VehicleCount++;

				if (isBeta)
					printf("\nV | %lX > XY: %0.1f %0.1f | N: %s", pBase, data->Location.X, data->Location.Y, name);
			}
			else if (strstr(name, "Pickup_C") || strstr(name, "PickUp") || strstr(name, "BP_Ammo") || strstr(name, "BP_QK") || strstr(name, "Wrapper")) {//Items
				if (!isPremium)
					continue;
				ItemData* data = &response.Items[response.ItemsCount];
				pvm(getA(pBase + 0x1a8) + 0x1b0, &exyz, sizeof(exyz));
				data->Location = WorldToScreen(exyz, POV, width, height);//World2Screen(vMat, exyz);
				if (data->Location.Z == 1.0f || data->Location.X > width + 100 || data->Location.X < -50)
					continue;
				data->Distance = getDistance(xyz, exyz);
				if (data->Distance > 200.0f)
					continue;
				strcpy(data->ItemName, name);
				if (response.ItemsCount >= maxitemsCount) {
					continue;
				}
				response.ItemsCount++;

				if (isBeta)
					printf("\nI | %lX > XY: %0.1f %0.1f | D:%0.1fm %s", pBase, data->Location.X, data->Location.Y, data->Distance, name);
			}
			else if (strstr(name, "BP_AirDropPlane_C") || strstr(name, "PlayerDeadInventoryBox_C") || strstr(name, "BP_AirDropBox_C")) {//Items
				if (!isPremium)
					continue;

				ItemData* data = &response.Items[response.ItemsCount];
				pvm(getA(pBase + 0x1a8) + 0x1b0, &exyz, sizeof(exyz));
				data->Location = WorldToScreen(exyz, POV, width, height);//World2Screen(vMat, exyz);
				if (data->Location.Z == 1.0f || data->Location.X > width + 100 || data->Location.X < -50)
					continue;
				data->Distance = getDistance(xyz, exyz);
				strcpy(data->ItemName, name);
				if (response.ItemsCount >= maxitemsCount) {
					continue;
				}
				response.ItemsCount++;

				if (isBeta)
					printf("\nSp | %lX > XY: %0.1f %0.1f | D:%0.1fm %s", pBase, data->Location.X, data->Location.Y, data->Distance, name);
			}
			else if (strstr(name, "BP_Grenade_Shoulei_C") || strstr(name, "BP_Grenade_Burn_C")) {//Grenade Warning
			if (!isPremium)
				continue;
			GrenadeData* data = &response.Grenade[response.GrenadeCount];
			pvm(getA(pBase + 0x1a8) + 0x1b0, &exyz, sizeof(exyz));
			data->Location = WorldToScreen(exyz, POV, width, height);//World2Screen(vMat, exyz);

			data->Distance = getDistance(xyz, exyz);
			if (data->Distance > 150.0f)
				continue;
			if (strstr(name, "Shoulei"))
				data->type = 1;
			else
				data->type = 2;
			if (response.GrenadeCount >= maxgrenadeCount) {
				continue;
			}
			response.GrenadeCount++;


			if (isBeta)
				printf("\nGW | %lX > XY: %0.1f %0.1f | D:%0.1fm %d", pBase, data->Location.X, data->Location.Y, data->Distance, name);
			}


		}

		if(response.PlayerCount+response.ItemsCount+response.VehicleCount+response.GrenadeCount + response.GrenadeCount >0)
			response.Success = true;

		if (isBeta) {
			printf("\nPlayers: %d | Vehicle: %d | Items: %d ", response.PlayerCount,response.VehicleCount,response.ItemsCount);
			break;
		}
		else {
			send((void*)&response, sizeof(response));
			if ((firing || ads) && nearest > 0 /*&& aimObj == 1 */) {
				// p_write(yawPitch, &pointingAngle, 8);
				 //thread Modification(freezeThread, yawPitch - 4  , yawPitch , pointingAngle.X,pointingAngle.Y);
                 //Modification.join();
                 if (Aim.objAddr == 0 || aimlock == 0)
                 {
                    Aim.objAddr = targetAim; 
                 }
                 
                 if(Aim.objAddr){
                            uintptr_t pBase = Aim.objAddr;
                            pvm(getA(pBase + 0x6228) + 0x1b0, &exyz, sizeof(exyz));
                            float Distance = getDistance(xyz, exyz);
                            if (aimFor == 1){
                                exyzAim = exyz;
                                Aim.objAddr = pBase;
                              //  Aim.ScreenDistance = get2dDistance(width/2, height/2, data->HeadLocation.X,data->HeadLocation.Y);
                                Aim.WorldDistance = Distance;
							    //pointingAngle = getPointingAngle(xyz, exyz, data->Distance);
							}

							else if (aimFor == 2) {

								uintptr_t boneAddr = getA(pBase + 0x420);
								struct D3DMatrix baseMatrix = getOMatrix(boneAddr + 0x1a0);
								boneAddr = getA(boneAddr + 0x7a8);
								struct D3DMatrix oMatrix = getOMatrix(boneAddr + 4 * 48);

								//pointingAngle = getPointingAngle(xyz, mat2Cord(oMatrix, baseMatrix), data->Distance);
                                exyzAim = mat2Cord(oMatrix, baseMatrix);
                                Aim.objAddr = pBase;
                               // Aim.ScreenDistance = get2dDistance(width/2, height/2, data->HeadLocation.X,data->HeadLocation.Y);
                                Aim.WorldDistance = Distance;
							}
							else {
								uintptr_t boneAddr = getA(pBase + 0x420);
								struct D3DMatrix baseMatrix = getOMatrix(boneAddr + 0x1a0);
								boneAddr = getA(boneAddr + 0x7a8);
								struct D3DMatrix oMatrix = getOMatrix(boneAddr + 2 * 48);
								//pointingAngle = getPointingAngle(xyz, mat2Cord(oMatrix, baseMatrix), data->Distance);
                                exyzAim = mat2Cord(oMatrix, baseMatrix);
                                Aim.objAddr = pBase;
                              //  Aim.ScreenDistance = get2dDistance(width/2, height/2, data->HeadLocation.X,data->HeadLocation.Y);
                                Aim.WorldDistance = Distance;
							}
							
							Aim.Ox = exyzAim;
                 }
                 
                 ads2 = 1;
                 if(firing)
                    firing2 = 1;
                 else 
                    firing2 = 0;

			}
			else{
			   ads2 = 0;
			}
		}

	}
    ioctl(fd, EVIOCGRAB, UNGRAB);


	if (isBeta)
		puts("\n\nScript Completed ");
}

uintptr_t getMatrix(uintptr_t base)
{
	if (strstr(version, "com.tencent.ig"))
		return getA(getA(base + 0xbbf7070) + 0xc0);
	return getA(getA(base + 0xbbf7070) + 0xc0);
}

uintptr_t getEntityAddr(uintptr_t base)
{
	if (strstr(version, "com.tencent.ig"))
		return getA(getA(getA(getA(base + 0xbbf7070) + 0x58) + 0x78) + 0x30) + 0xa0;
	return getA(getA(getA(getA(base + 0xbbf7070) + 0x58) + 0x78) + 0x30) + 0xa0;
}

uintptr_t getGWorld(uintptr_t base)
{
	if (strstr(version, "com.tencent.ig"))
		return getA(getA(getA(base + 0xbbf7070) + 0x58) + 0x78);
	return getA(getA(getA(base + 0xbbf7070) + 0x58) + 0x78);
}

char* getNameByte(uintptr_t address)
{
	char static lj[64];
	memset(lj, 0, 64);
	unsigned short int nameI[32];
	pvm(address, nameI, sizeof(nameI));
	char s[10];
	int i;
	for (i = 0; i < 32; i++)
	{
		if (nameI[i] == 0)
			break;
		sprintf(s, "%d:", nameI[i]);
		strcat(lj, s);
	}
	lj[63] = '\0';

	return lj;
}


PlayerBone getPlayerBone(uintptr_t pBase, struct D3DMatrix viewMatrix, MinimalViewInfo POV)
{
	PlayerBone b;
	b.isBone = true;
	struct D3DMatrix oMatrix;
	uintptr_t boneAddr = getA(pBase + 0x420);
	struct D3DMatrix baseMatrix = getOMatrix(boneAddr + 0x1a0);
	int bones[] = { 4, 3, 0,11, 32, 12, 33, 63, 62, 52, 56, 53, 57, 54, 58 };
	boneAddr = getA(boneAddr + 0x7a8);

	//neck 0
	oMatrix = getOMatrix(boneAddr + (bones[0] + 1) * 48);
	b.neck = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//cheast 1
	oMatrix = getOMatrix(boneAddr + (bones[1] + 1) * 48);
	b.cheast = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//pelvis 2
	oMatrix = getOMatrix(boneAddr + (bones[2] + 1) * 48);
	b.pelvis = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//lSh 3
	oMatrix = getOMatrix(boneAddr + (bones[3] + 1) * 48);
	b.lSh = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//rSh 4
	oMatrix = getOMatrix(boneAddr + (bones[4] + 1) * 48);
	b.rSh = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//lElb 5
	oMatrix = getOMatrix(boneAddr + (bones[5] + 1) * 48);
	b.lElb = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//rElb 6
	oMatrix = getOMatrix(boneAddr + (bones[6] + 1) * 48);
	b.rElb = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//lWr 7
	oMatrix = getOMatrix(boneAddr + (bones[7] + 1) * 48);
	b.lWr = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//rWr 8
	oMatrix = getOMatrix(boneAddr + (bones[8] + 1) * 48);
	b.rWr = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//lTh 9
	oMatrix = getOMatrix(boneAddr + (bones[9] + 1) * 48);
	b.lTh = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//rTh 10
	oMatrix = getOMatrix(boneAddr + (bones[10] + 1) * 48);
	b.rTh = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//lKn 11
	oMatrix = getOMatrix(boneAddr + (bones[11] + 1) * 48);
	b.lKn = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//rKn 12
	oMatrix = getOMatrix(boneAddr + (bones[12] + 1) * 48);
	b.rKn = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//lAn 13 
	oMatrix = getOMatrix(boneAddr + (bones[13] + 1) * 48);
	b.lAn = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);
	//rAn 14
	oMatrix = getOMatrix(boneAddr + (bones[14] + 1) * 48);
	b.rAn = WorldToScreenMain(mat2Cord(oMatrix, baseMatrix), POV, width, height);

	return b;
}
PlayerWeapon getPlayerWeapon(uintptr_t base) {
	PlayerWeapon p;
	uintptr_t addr[3];
	pvm(getA(base + 0x198), addr, sizeof(addr));

	if (isValid64(addr[0]) && getI(addr[0] + 0x134) == 2) {
		p.isWeapon = true;
		p.id = getI(getA(addr[0] + 0xf68) + 0x170);
		p.ammo = getI(addr[0] + 0xe28);
	}
	else if (isValid64(addr[1]) && getI(addr[1] + 0x134) == 2) {
		p.isWeapon = true;
		p.id = getI(getA(addr[1] + 0xf68) + 0x170);
		p.ammo = getI(addr[1] + 0xe28);
	}
	else if (isValid64(addr[2]) && getI(addr[2] + 0x134) == 2) {
		p.isWeapon = true;
		p.id = getI(getA(addr[2] + 0xf68) + 0x170);
		p.ammo = getI(addr[2] + 0xe28);
	}

	return p;
}