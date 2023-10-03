#include <sys/uio.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include<math.h>
#include <stdlib.h>
#include <assert.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <sys/un.h>
#include <time.h>
#include <ctype.h>
#include <iostream>



#if defined(__arm__)
int process_vm_readv_syscall = 376;
int process_vm_writev_syscall = 377;
#elif defined(__aarch64__)
int process_vm_readv_syscall = 270;
int process_vm_writev_syscall = 271;
#elif defined(__i386__)
int process_vm_readv_syscall = 347;
int process_vm_writev_syscall = 348;
#else
int process_vm_readv_syscall = 310;
int process_vm_writev_syscall = 311;
#endif

#define LEN sizeof(struct MAPS)

struct Vec4 {
    float  X, Y, Z, W;
};
struct Vec3 {
	float X;
	float Y;
	float Z;

	Vec3() {
		this->X = 0;
		this->Y = 0;
		this->Z = 0;
	}

	Vec3(float x, float y, float z) {
		this->X = x;
		this->Y = y;
		this->Z = z;
	}

	static Vec3 Zero() {
		return Vec3(0.0f, 0.0f, 0.0f);
	}

	static Vec3 Up() {
		return Vec3(0.0f, 1.0f, 0.0f);
	}

	static Vec3 Down() {
		return Vec3(0.0f, -1.0f, 0.0f);
	}

	static Vec3 Back() {
		return Vec3(0.0f, 0.0f, -1.0f);
	}

	static Vec3 Forward() {
		return Vec3(0.0f, 0.0f, 1.0f);
	}

	static Vec3 Left() {
		return Vec3(-1.0f, 0.0f, 0.0f);
	}

	static Vec3 Right() {
		return Vec3(1.0f, 0.0f, 0.0f);
	}

	float& operator[](int i) {
		return ((float*)this)[i];
	}

	float operator[](int i) const {
		return ((float*)this)[i];
	}

	bool operator==(const Vec3& src) const {
		return (src.X == X) && (src.Y == Y) && (src.Z == Z);
	}

	bool operator!=(const Vec3& src) const {
		return (src.X != X) || (src.Y != Y) || (src.Z != Z);
	}

	Vec3& operator+=(const Vec3& v) {
		X += v.X;
		Y += v.Y;
		Z += v.Z;
		return *this;
	}

	Vec3& operator-=(const Vec3& v) {
		X -= v.X;
		Y -= v.Y;
		Z -= v.Z;
		return *this;
	}

	Vec3& operator*=(float fl) {
		X *= fl;
		Y *= fl;
		Z *= fl;
		return *this;
	}

	Vec3& operator*=(const Vec3& v) {
		X *= v.X;
		Y *= v.Y;
		Z *= v.Z;
		return *this;
	}

	Vec3& operator/=(const Vec3& v) {
		X /= v.X;
		Y /= v.Y;
		Z /= v.Z;
		return *this;
	}

	Vec3& operator+=(float fl) {
		X += fl;
		Y += fl;
		Z += fl;
		return *this;
	}

	Vec3& operator/=(float fl) {
		X /= fl;
		Y /= fl;
		Z /= fl;
		return *this;
	}

	Vec3& operator-=(float fl) {
		X -= fl;
		Y -= fl;
		Z -= fl;
		return *this;
	}

	Vec3& operator=(const Vec3& vOther) {
		X = vOther.X;
		Y = vOther.Y;
		Z = vOther.Z;
		return *this;
	}

	Vec3 operator-(void) const {
		return Vec3(-X, -Y, -Z);
	}

	Vec3 operator+(const Vec3& v) const {
		return Vec3(X + v.X, Y + v.Y, Z + v.Z);
	}

	Vec3 operator-(const Vec3& v) const {
		return Vec3(X - v.X, Y - v.Y, Z - v.Z);
	}

	Vec3 operator*(float fl) const {
		return Vec3(X * fl, Y * fl, Z * fl);
	}

	Vec3 operator*(const Vec3& v) const {
		return Vec3(X * v.X, Y * v.Y, Z * v.Z);
	}

	Vec3 operator/(float fl) const {
		return Vec3(X / fl, Y / fl, Z / fl);
	}

	Vec3 operator/(const Vec3& v) const {
		return Vec3(X / v.X, Y / v.Y, Z / v.Z);
	}

	static float Dot(Vec3 lhs, Vec3 rhs) {
		return (((lhs.X * rhs.X) + (lhs.Y * rhs.Y)) + (lhs.Z * rhs.Z));
	}

	float sqrMagnitude() const {
		return (X * X + Y * Y + Z * Z);
	}

	float Magnitude() const {
		return sqrt(sqrMagnitude());
	}

	static float Distance(Vec3 a, Vec3 b) {
		Vec3 vector = Vec3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
		return sqrt(((vector.X * vector.X) + (vector.Y * vector.Y)) + (vector.Z * vector.Z));
	}
};
struct Vec2 {
    float X, Y;
    
    Vec2() {
		this->X = 0;
		this->Y = 0;
		
	}

	Vec2(float x, float y) {
		this->X = x;
		this->Y = y;
		
	}
};


struct D3DMatrix

{
    float _11, _12, _13, _14;

    float _21, _22, _23, _24;

    float _31, _32, _33, _34;

    float _41, _42, _43, _44;

};



struct Vec4 rot;
struct Vec3 scale, tran;

//deta
int height = 1080;
int width = 2340;
int pid = 0;
int isBeta, nByte;
float mx = 0, my = 0, mz = 0;

struct MAPS
{
    long int fAddr;
    long int lAddr;
    struct MAPS* next;
    long int addr;
    long int taddr;
    int type;
};
struct Ulevel {
    uintptr_t addr;
    int size;
};
typedef struct MAPS* PMAPS;

#define SIZE sizeof(uintptr_t)

