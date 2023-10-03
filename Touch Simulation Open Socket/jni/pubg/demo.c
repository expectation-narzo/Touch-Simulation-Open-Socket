#include <stdio.h>
#include <time.h>
#include "base.h" //请前往设置base编码集

int main(int argc, char *argv[])
{
	
  FILE *fp;
	char kami[41];
    if ((fp = fopen("/storage/emulated/0/t", "r")) != NULL){ 
	fgets(kami, 40, fp);
	fclose(fp);
    }else{
    	return 0;
    }

    const static char *appid = "13938";
	const static char *key = "666";
	const static int num =666;
	const static char bmj[65] ="Mtw18nNgDLVbY3S+opiHGlj5TIOe7WhqcdZfuBXy0CJKFU4amRzvP96AsrxQ2Ek/";
	const static char *zdyimei = "a47036c26dcddc7a";
	const static char *zdykami = "159982006195932324";

	//主函数，启动就在这
	setbase(bmj);				//设置编码集
	char *run;					//返回值
	char *jm;					//解密
	char *km = yjjm(zdykami);	//卡密
	char *imei = yjjm(zdyimei); //iemi
	char url[256];
	sprintf(url, "user/kmdenglu.php?appid=%s&km=%s&imei=%s", appid, km, imei);
	httpget(host, url, &run);
	jm = Decbase64(run); //解密
	free(run);
	free(km);
	free(imei);
	printf("%s ",zdykami);
	
	
	switch (Check(jm, zdykami, zdyimei))
	{
	case 1:
		puts("校验成功");//自行把对应事件的处理方法写在下面
		break;
	case -2:
		puts("校验失败");//比如在失败后面加return -1;
		break;
	case -1:
		printf("登录失败:%s\n", jm);
		break;
	default:
		puts("数据异常");
		break;
	}
	free(jm);
	return 0;
}

