 #include "socket.h"
 #include "init.h"
 #include <thread>
#define PI 3.141592653589793238

typedef uintptr_t kaddr;

float read_Float(uintptr_t addr);	// 读取float类型
int read_Dword(uintptr_t addr);	// 读取dword类型
long int read_Pointer(uintptr_t addr);	// 读取指针
int amend_Float(uintptr_t  addr, float value);
int amend_Dword(uintptr_t addr, int value);

ssize_t process_v(pid_t __pid,   struct iovec* __local_iov, unsigned long __local_iov_count, struct iovec* __remote_iov, unsigned long __remote_iov_count, unsigned long __flags) {
	return syscall(process_vm_readv_syscall, __pid, __local_iov, __local_iov_count, __remote_iov, __remote_iov_count, __flags);
}

ssize_t process_v(pid_t __pid, const struct iovec *__local_iov, unsigned long __local_iov_count,
				  const struct iovec *__remote_iov, unsigned long __remote_iov_count,
				  unsigned long __flags, bool iswrite)
{
	return syscall((iswrite ? process_vm_writev_syscall : process_vm_readv_syscall), __pid,
				   __local_iov, __local_iov_count, __remote_iov, __remote_iov_count, __flags);
}

/* 进程读写内存 */
bool pvm1(void *address, void *buffer, size_t size, bool iswrite)
{
	struct iovec local[1];
	struct iovec remote[1];

	local[0].iov_base = buffer;
	local[0].iov_len = size;
	remote[0].iov_base = address;
	remote[0].iov_len = size;

	if (pid < 0)
	{
		return false;
	}

	ssize_t bytes = process_v(pid, local, 1, remote, 1, 0, iswrite);
	return bytes == size;
}

/* 读内存 */
bool vm_readv(void* address, void *buffer, size_t size)
{
	return pvm1(reinterpret_cast < void *>(address), buffer, size, false);
}

template <typename T>
T Read(kaddr address) {
	T data;
	vm_readv(reinterpret_cast<void*>(address), reinterpret_cast<void*>(&data), sizeof(T));
	return data;
}

struct FRotator {
	float Pitch;
	float Yaw;
	float Roll;
};

struct MinimalViewInfo {
	Vec3 Location;
	Vec3 LocationLocalSpace;
	FRotator Rotation;
	float FOV;
};

struct FMatrix {
	float M[4][4];
};

int pvm(uintptr_t address, void* buffer,int size) {
	struct iovec local[1];
	struct iovec remote[1];

	local[0].iov_base = (void*)buffer;
	local[0].iov_len = size;
	remote[0].iov_base = (void*)address;
	remote[0].iov_len = size;

ssize_t bytes = process_vm_readv(pid, local, 1, remote, 1, 0);
	return bytes == size;
}

FMatrix RotToMatrix(FRotator rotation) {
	float radPitch = rotation.Pitch * ((float)PI / 180.0f);
	float radYaw = rotation.Yaw * ((float)PI / 180.0f);
	float radRoll = rotation.Roll * ((float)PI / 180.0f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	FMatrix matrix;

	matrix.M[0][0] = (CP * CY);
	matrix.M[0][1] = (CP * SY);
	matrix.M[0][2] = (SP);
	matrix.M[0][3] = 0;

	matrix.M[1][0] = (SR * SP * CY - CR * SY);
	matrix.M[1][1] = (SR * SP * SY + CR * CY);
	matrix.M[1][2] = (-SR * CP);
	matrix.M[1][3] = 0;

	matrix.M[2][0] = (-(CR * SP * CY + SR * SY));
	matrix.M[2][1] = (CY * SR - CR * SP * SY);
	matrix.M[2][2] = (CR * CP);
	matrix.M[2][3] = 0;

	matrix.M[3][0] = 0;
	matrix.M[3][1] = 0;
	matrix.M[3][2] = 0;
	matrix.M[3][3] = 1;

	return matrix;
}

Vec3 WorldToScreen(Vec3 worldLocation, MinimalViewInfo camViewInfo, int width, int height) {
    FMatrix tempMatrix = RotToMatrix(camViewInfo.Rotation);

    Vec3 vAxisX(tempMatrix.M[0][0], tempMatrix.M[0][1], tempMatrix.M[0][2]);
    Vec3 vAxisY(tempMatrix.M[1][0], tempMatrix.M[1][1], tempMatrix.M[1][2]);
    Vec3 vAxisZ(tempMatrix.M[2][0], tempMatrix.M[2][1], tempMatrix.M[2][2]);

    Vec3 vDelta = worldLocation - camViewInfo.Location;

    Vec3 vTransformed(Vec3::Dot(vDelta, vAxisY), Vec3::Dot(vDelta, vAxisZ), Vec3::Dot(vDelta, vAxisX));

    if (vTransformed.Z < 1.0f) {
        vTransformed.Z = 1.0f;
    }

    float fov = camViewInfo.FOV;
    float screenCenterX = (width / 2.0f);
    float screenCenterY = (height / 2.0f);

    return Vec3(
        (screenCenterX + vTransformed.X * (screenCenterX / tanf(fov * ((float)PI / 360.0f))) / vTransformed.Z),
        (screenCenterY - vTransformed.Y * (screenCenterX / tanf(fov * ((float)PI / 360.0f))) / vTransformed.Z),
        vTransformed.Z
    );
}

Vec2 WorldToScreenMain(Vec3 worldLocation, MinimalViewInfo camViewInfo, int width, int height) {
    FMatrix tempMatrix = RotToMatrix(camViewInfo.Rotation);

    Vec3 vAxisX(tempMatrix.M[0][0], tempMatrix.M[0][1], tempMatrix.M[0][2]);
    Vec3 vAxisY(tempMatrix.M[1][0], tempMatrix.M[1][1], tempMatrix.M[1][2]);
    Vec3 vAxisZ(tempMatrix.M[2][0], tempMatrix.M[2][1], tempMatrix.M[2][2]);

    Vec3 vDelta = worldLocation - camViewInfo.Location;

    Vec3 vTransformed(Vec3::Dot(vDelta, vAxisY), Vec3::Dot(vDelta, vAxisZ), Vec3::Dot(vDelta, vAxisX));

    if (vTransformed.Z < 1.0f) {
        vTransformed.Z = 1.0f;
    }

    float fov = camViewInfo.FOV;
    float screenCenterX = (width / 2.0f);
    float screenCenterY = (height / 2.0f);

    return Vec2(
        (screenCenterX + vTransformed.X * (screenCenterX / tanf(fov * ((float)PI / 360.0f))) / vTransformed.Z),
        (screenCenterY - vTransformed.Y * (screenCenterX / tanf(fov * ((float)PI / 360.0f))) / vTransformed.Z)
    );
}

struct D3DMatrix ToMatrixWithScale(struct Vec3 translation,struct Vec3 scale,struct Vec4 rot)
 {
struct D3DMatrix m;
 m._41 = translation.X;
 m._42 = translation.Y;
 m._43 = translation.Z;

float x2 = rot.X + rot.X;
float y2 = rot.Y + rot.Y;
float z2 = rot.Z + rot.Z;


float xx2 = rot.X * x2;
float yy2 = rot.Y * y2;
float zz2 = rot.Z * z2;

m._11 = (1.0f - (yy2 + zz2)) * scale.X;
m._22 = (1.0f - (xx2 + zz2)) * scale.Y;
m._33 = (1.0f - (xx2 + yy2)) * scale.Z;


float yz2 = rot.Y * z2;
float wx2 = rot.W * x2;

m._32 = (yz2 - wx2) * scale.Z;
m._23 = (yz2 + wx2) * scale.Y;

float xy2 = rot.X * y2;
float wz2 = rot.W * z2;

m._21 = (xy2 - wz2) * scale.Y;
m._12 = (xy2 + wz2) * scale.X;


float xz2 = rot.X * z2;
float wy2 = rot.W * y2;

m._31 = (xz2 + wy2) * scale.Z;
m._13 = (xz2 - wy2) * scale.X;

m._14 = 0.0f;
m._24 = 0.0f;
m._34 = 0.0f;
m._44 = 1.0f;

return m;
}
struct Vec3 mat2Cord(struct D3DMatrix pM1,struct D3DMatrix pM2){
struct  Vec3 pOut;
pOut.X = pM1._41 * pM2._11 + pM1._42 * pM2._21 + pM1._43 * pM2._31 + pM1._44 * pM2._41;
pOut.Y = pM1._41 * pM2._12 + pM1._42 * pM2._22 + pM1._43 * pM2._32 + pM1._44 * pM2._42;
pOut.Z = pM1._41 * pM2._13 + pM1._42 * pM2._23 + pM1._43 * pM2._33 + pM1._44 * pM2._43;

return pOut;
}

float getF(uintptr_t address) ;
//自瞄专用
struct FTransform
{
	Vec4 Rotation;
	Vec3 Translation;
	Vec3 Scale3D;
};

FTransform ReadFTransform(uintptr_t address);
//获取骨骼3D
Vec3 getBoneXYZ(uintptr_t humanAddr, uintptr_t boneAddr, int Part)
{
	// 获取Bone数据
	FTransform Bone = ReadFTransform(boneAddr + Part * 48);
	// 获取Actor数据
	FTransform Actor = ReadFTransform(humanAddr);



	D3DMatrix Bone_Matrix = ToMatrixWithScale( Bone.Translation, Bone.Scale3D,Bone.Rotation);

	D3DMatrix Component_ToWorld_Matrix =ToMatrixWithScale( Actor.Translation, Actor.Scale3D,Actor.Rotation);

	Vec3 result = mat2Cord(Bone_Matrix, Component_ToWorld_Matrix);

	return result;
}



FTransform ReadFTransform(uintptr_t address)
{
	FTransform Result;
	Result.Rotation.X = getF(address);	// Rotation_X
	Result.Rotation.Y = getF(address + 4);	// Rotation_y
	Result.Rotation.Z = getF(address + 8);	// Rotation_z
	Result.Rotation.W = getF(address + 12);	// Rotation_w
	Result.Translation.X = getF(address + 16);	// /Translation_X
	Result.Translation.Y = getF(address + 20);	// Translation_y
	Result.Translation.Z = getF(address + 24);	// Translation_z
	Result.Scale3D.X = getF(address + 32);;	// Scale_X
	Result.Scale3D.Y = getF(address + 36);;	// Scale_y
	Result.Scale3D.Z = getF(address + 40);;	// Scale_z
	return Result;
}






uintptr_t getBase(){
    FILE *fp;
    uintptr_t addr = 0;
    char filename[40], buffer[1024];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    fp = fopen(filename, "rt");
    if (fp != NULL) {
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strstr(buffer, "libUE4.so")) {
                addr = (uintptr_t)strtoull(buffer, NULL, 16);
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}




typedef char PACKAGENAME;
int getPID(PACKAGENAME * PackageName)
{
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	FILE *fp = NULL;
	char filepath[256];			// 大小随意，能装下cmdline文件的路径即可
	char filetext[128];			// 大小随意，能装下要识别的命令行文本即可
	dir = opendir("/proc");		// 打开路径
	if (NULL != dir)
	{
		while ((ptr = readdir(dir)) != NULL)
		{						// 循环读取路径下的每一个文件/文件夹
			// 如果读取到的是"."或者".."则跳过，读取到的不是文件夹名字也跳过
			if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
				continue;
			if (ptr->d_type != DT_DIR)
				continue;
			sprintf(filepath, "/proc/%s/cmdline", ptr->d_name);	// 生成要读取的文件的路径
			fp = fopen(filepath, "r");	// 打开文件
			if (NULL != fp)
			{
				fgets(filetext, sizeof(filetext), fp);	// 读取文件
				if (strcmp(filetext, PackageName) == 0)
				{
					// puts(filepath);
					// printf("packagename:%s\n",filetext);
					break;
				}
				fclose(fp);
			}
		}
	}
	if (readdir(dir) == NULL)
	{
		return 0;
	}
	closedir(dir);				// 关闭路径
	return atoi(ptr->d_name);
}


pid_t getPid(char * name){
	char text[69];
	pid_t pid = 0;
	sprintf(text,"pidof %s",name);
FILE *chkRun = popen(text, "r");
	if (chkRun){
		char output[10];
		fgets(output ,10,chkRun);
		pclose(chkRun);
		pid= atoi(output);
		}
	if (pid < 10) {
		DIR* dir = NULL;
		struct dirent* ptr = NULL;
		FILE* fp = NULL;
		char filepath[256];
		char filetext[128];
		dir = opendir("/proc");
		if (NULL != dir)
		{
			while ((ptr = readdir(dir)) != NULL)
			{
				if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
					continue;
				if (ptr->d_type != DT_DIR)
					continue;
				sprintf(filepath, "/proc/%s/cmdline", ptr->d_name);
				fp = fopen(filepath, "r");
				if (NULL != fp)
				{
					fgets(filetext, sizeof(filetext), fp);


					if (strcmp(filetext, name) == 0)
					{
						fclose(fp);
						break;
					}
					fclose(fp);
				}
			}
		}
		if (readdir(dir) == NULL)
		{
			closedir(dir);
			return 0;
		}
		closedir(dir);
		pid= atoi(ptr->d_name);
	}
		return pid;
}


// 计算旋转坐标
Vec2 rotateCoord(float angle, float objectRadar_x, float objectRadar_y)
{
	Vec2 radarCoordinate;
	float s = sin(angle * PI / 180);
	float c = cos(angle * PI / 180);
	radarCoordinate.X = objectRadar_x * c + objectRadar_y * s;
	radarCoordinate.Y = -objectRadar_x * s + objectRadar_y * c;
	return radarCoordinate;
}

float get2dDistance(float x, float y, float x1, float y1)
{
	float xx1 = x - x1;
	float yy2 = y - y1;
	// 取平方根
	return sqrt(xx1 * xx1 + yy2 * yy2);
}

float get_3D_Distance(float Self_x, float Self_y, float Self_z, float Object_x, float Object_y, float Object_z)
{
	float x, y, z;
	x = Self_x - Object_x;
	y = Self_y - Object_y;
	z = Self_z - Object_z;
	// 求平方根
	return (float)(sqrt(x * x + y * y + z * z));
}

int isValidItem(int id) {
	if (id >= 100000 && id < 999999)
		return 1;
    return 0;
}
float getF(uintptr_t address) {
	float buff;
	pvm(address, &buff, 4);
	return buff;
}
uintptr_t getA(uintptr_t address) {
	uintptr_t buff;
    pvm(address, &buff, SIZE);
    return buff;
}
int getI(uintptr_t address) {
	int buff;
	pvm(address, &buff, 4);
	return buff;
}

int handle;
//读
float read_Float(uintptr_t addr)
{
	float var = 0;
	pread64(handle, &var, 4, addr);
	return var;
}

int read_Dword(uintptr_t addr)
{

	int var = 0;
	pread64(handle, &var, 4, addr);
	return var;
}

long int read_Pointer(uintptr_t addr)
{
	long int var = 0;
	pread64(handle, &var, 4, addr);
	return var;
}

//写

int amend_Dword(uintptr_t addr,int value )
{
	pwrite64(handle, &value, 4, addr);
	return 0;
}

int amend_Float(uintptr_t addr, float value)
{
	pwrite64(handle, &value, 4, addr);
	return 0;
}

int isValid64(uintptr_t addr) {
    if (addr < 0x1000000000 || addr>0xefffffffff || addr % SIZE != 0)
        return 0;
    return 1;
}
int isValid32(uintptr_t addr) {
    if (addr < 0x10000000 || addr>0xefffffff || addr % SIZE != 0)
        return 0;
    return 1;
}


void Ornginitialize(){
 
     system("echo 8192 > /proc/sys/fs/inotify/max_user_watches");
     system("echo 16384 > /proc/sys/fs/inotify/max_queued_events");
     system("echo 128 > /proc/sys/fs/inotify/max_user_instances");
	 int ipid = getPID(version);
    	if (ipid == 0)
    	{
    		puts("Process acquisition failed!");
    		exit(1);
    	}
	char lj[64];
    sprintf(lj, "/proc/%d/mem", ipid);
    handle = open(lj, O_RDWR);
     	if (handle == -1)
     	{
     		puts("mem reading failed");
     		exit(1);
     	}
     	lseek(handle, 0, SEEK_SET);

}

PMAPS pMap_A = nullptr , pMap_ALL = nullptr;
/* 读取MAPS文件 */
PMAPS readmaps() {
	PMAPS pHead = nullptr;
	PMAPS pNew = nullptr;
	PMAPS pEnd = nullptr;
	pEnd = pNew = (PMAPS)malloc(LEN);
	int i = 0, flag = 1;
	char lj[50], buff[256];
	snprintf(lj, sizeof(lj), "/proc/%d/maps", pid);
	FILE *fp = fopen(lj, "r");
	if (fp == nullptr) {
		printf("open maps fail!\n");
		return nullptr;
	}
	while (!feof(fp)) {
		fgets(buff, sizeof(buff), fp);
		if (strstr(buff, "rw") != nullptr && !feof(fp) && (strlen(buff) < 42)) {
			sscanf(buff, "%lx-%lx", &pNew->addr, &pNew->taddr); // 这里使用lx是为了能成功读取特别长的地址
			flag = 1;
		} else {
			flag = 0;
		}
		if (flag == 1) {
			i++;
			if (i == 1) {
				pNew->next = nullptr;
				pEnd = pNew;
				pHead = pNew;
			} else {
				pNew->next = nullptr;
				pEnd->next = pNew;
				pEnd = pNew;
			}
			pNew = (PMAPS)malloc(LEN); //分配内存
		}
	}
	free(pNew); //将多余的空间释放
	fclose(fp); //关闭文件指针
	return pHead;
}

PMAPS readmaps_all() {
	PMAPS pHead = nullptr;
	PMAPS pNew = nullptr;
	PMAPS pEnd = nullptr;
	pEnd = pNew = (PMAPS)malloc(LEN);
	int i = 0,flag=1;
	char lj[64], buff[256];
	snprintf(lj, sizeof(lj), "/proc/%d/maps", pid);
	FILE *fp = fopen(lj, "r");
	if (fp == nullptr) {
		printf("open maps fail!\n");
		return nullptr;
	}
	while (!feof(fp)) {
		fgets(buff,sizeof(buff),fp);//读取一行
		if (strstr(buff, "---p") != nullptr && !feof(fp)) {
			sscanf(buff, "%lx-%lx", &pNew->addr, &pNew->taddr); //这里使用lx是为了能成功读取特别长的地址
			flag = 1;
		} else {
			flag = 0;
		}
		if (flag == 1) {
			i++;
			if (i == 1) {
				pNew->next = nullptr;
				pEnd = pNew;
				pHead = pNew;
			} else {
				pNew->next = nullptr;
				pEnd->next = pNew;
				pEnd = pNew;
			}
			pNew = (PMAPS) malloc(LEN); //分配内存
		}
	}
	free(pNew); //将多余的空间释放
	fclose(fp); //关闭文件指针
	return pHead;
}




long int getA2(uintptr_t addr) {
	if(addr < 0xFFFFFF){
		return 0;
	}
	int isPass = 0;
	uintptr_t var[4] = {0};
	memset(var, 0, 4);
	pvm(addr, &var, 4);
    //pvm(address, buff, 4);
	PMAPS pTemp = nullptr;
	pTemp = pMap_A;
	//pTemp= readmaps();
	while (pTemp != nullptr && isPass == 0) {
		long int size = (pTemp->taddr - pTemp->addr); //计算内存大小
		// 判断指针是否在正常范围
		if (pTemp->addr <= var[0] && pTemp->taddr > var[0] && size >= 0x500) {
			PMAPS pTemp2 = nullptr;
			pTemp2 = pMap_ALL;
		//	pTemp2 = readmaps_all();
			while (pTemp2 != nullptr) {
				// 判断A内存的起始地址是否等于 O内存的结束地址
				if (pTemp->addr == pTemp2->taddr) {
					pTemp2 = pTemp2->next;
					// 判断A内存的结束地址是否等于 O内存的起始地址
					if (pTemp->taddr == pTemp2->addr) {
						return 0;
					}
					break;
				}
				pTemp2 = pTemp2->next;
			}
			isPass = 1;
		}
		pTemp = pTemp->next;
	}
	return isPass == 1 ? var[0] : 0;
}



float getDistance(struct Vec3 mxyz,struct Vec3 exyz){
return sqrt ((mxyz.X-exyz.X)*(mxyz.X-exyz.X)+(mxyz.Y-exyz.Y)*(mxyz.Y-exyz.Y)+(mxyz.Z-exyz.Z)*(mxyz.Z-exyz.Z))/100;
	
}

struct Vec3 World2Screen(struct D3DMatrix viewMatrix, struct Vec3 pos) {
	struct Vec3 screen;
	float screenW = (viewMatrix._14 * pos.X) + (viewMatrix._24 * pos.Y) + (viewMatrix._34 * pos.Z) + viewMatrix._44;

	if (screenW < 0.01f)
		screen.Z = 1;
	else
		screen.Z = 0;
	float screenX = (viewMatrix._11 * pos.X) + (viewMatrix._21 * pos.Y) + (viewMatrix._31 * pos.Z) + viewMatrix._41;
	float screenY = (viewMatrix._12 * pos.X) + (viewMatrix._22 * pos.Y) + (viewMatrix._32 * pos.Z) + viewMatrix._42;
	screen.Y = (height / 2) - (height / 2) * screenY / screenW;
	screen.X = (width / 2) + (width / 2) * screenX / screenW;
	return screen;

}
struct Vec2 World2ScreenMain(struct D3DMatrix viewMatrix, struct Vec3 pos) {
	struct Vec2 screen;
	float screenW = (viewMatrix._14 * pos.X) + (viewMatrix._24 * pos.Y) + (viewMatrix._34 * pos.Z) + viewMatrix._44;
	float screenX = (viewMatrix._11 * pos.X) + (viewMatrix._21 * pos.Y) + (viewMatrix._31 * pos.Z) + viewMatrix._41;
	float screenY = (viewMatrix._12 * pos.X) + (viewMatrix._22 * pos.Y) + (viewMatrix._32 * pos.Z) + viewMatrix._42;
	screen.Y = (height / 2) - (height / 2) * screenY / screenW;
	screen.X = (width / 2) + (width / 2) * screenX / screenW;
	return screen;

}
struct D3DMatrix getOMatrix(uintptr_t boneAddr){
    float oMat[11];
    pvm(boneAddr,&oMat,sizeof(oMat));
    rot.X=oMat[0];
	rot.Y=oMat[1];
	rot.Z=oMat[2];
	rot.W=oMat[3];
			
	tran.X=oMat[4];
	tran.Y=oMat[5];
	tran.Z=oMat[6];
			
	scale.X=oMat[8];
	scale.Y=oMat[9];
	scale.Z=oMat[10];
			
	return ToMatrixWithScale(tran,scale,rot);
}
char* getText(uintptr_t addr) {
	static char txt[42];
	memset(txt, 0, 42);
	char buff[41];
	pvm(addr + 4+SIZE, &buff, 41);
	int i;
	for (i = 0; i < 41; i++) {
		if (buff[i] == 0)
			break;

		txt[i] = buff[i];

		if (buff[i] == 67 && i > 10)
			break;

	}
	txt[i + 1] = '\0';
	return txt;
}
void dump(const uintptr_t gaddr, const int gsize, char* name) {
	char buff[0x100000];
	uintptr_t addr = gaddr;
	int size = gsize;
	FILE* fp = fopen(name, "w");
	while (size > 0) {
		if (size < 0x100000) {
			pvm(addr, buff, size);

			for (int i = 0; i < size; i++)
				fwrite(&buff[i], 1, 1, fp);

		}
		else {
			pvm(addr, buff, 0x100000);

			for (int i = 0; i < 0x100000; i++)
				fwrite(&buff[i], 1, 1, fp);
		}

		addr += 0x100000;
		size -= 0x100000;
	}
	fclose(fp);

}