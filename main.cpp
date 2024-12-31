#include <stdio.h>
#include <graphics.h>
#include "tools.h"
#include <time.h>
#include <math.h>
#include "vector2.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#define WIN_WIDTH	1000
#define	WIN_HEIGHT	600
enum 
{
	WAN_DOU, XIANG_RI_KUI, ZHI_WU_COUNT//素材中还有很多种植物，可以增加更多的植物
};
//设置了一系列的枚举常量，分别代表了豌豆、向日葵和植物的数量，他们的值分别是0、1、2，确切的来说是从0开始递增的。

IMAGE imgBg;
IMAGE imgBar5;
IMAGE imgCards[ZHI_WU_COUNT];
IMAGE* imgZhiWu[ZHI_WU_COUNT][20];
IMAGE imgBar;
IMAGE imgEnd;
IMAGE imgEnd1;

int curX, curY;//当前选中的植物，在移动过程中的位置
int curZhiWu;	//0未选中 1第一种植物

//植物的结构体，表示植物的信息
struct zhiwu {
	int type;			//0没有植物 1选择第一种植物...可以增加更多的植物
	int frameIndex;		//序列帧的序号
	bool catched;//是否被僵尸捕获
	int deadTime;//死亡计数器
	int timer;//计时器，用于判断向日葵好久出现一个阳光
	int x,y;//植物的位置
};
//植物的二维数组，表示植物的位置
struct zhiwu map[3][9];

enum { SUNSHINE_DOWN,SUNSHINE_GROUND,SUNSHINE_COLLECT,SUNSHINE_PRODUCT};//阳光的状态

//阳光的结构体，表示阳光的信息
struct sunshineBall {
	int x , y;//飘落过程位置 
	int frameIndex;//当前显示的帧序数
	int destY;//飘落终点y坐标，垂直飘落，不会改变x坐标
	bool used;//是否在使用
	int timer;//计时器
    float xoff;//每一帧的x偏移量
	float yoff;//每一帧的y偏移量
	float t;//贝塞尔曲线的时间点
	vector2 p1,p2,p3,p4;
	vector2 pCur;//当前阳光球的位置
	float speed;//飘落速度
	int status;//阳光的状态
};
struct sunshineBall balls[10];
IMAGE imgSunshineBall[29];
int sunshine=50;

struct zm {
    int x, y;
    int frameIndex;
    bool used;
    int speed;
    int row ;
    int blood; 
	bool dead;
	bool eating;//正在吃植物
};
struct zm zms[10];//此处可以自由设置僵尸的数量，这里设置了10个僵尸
int zmd = 0;
IMAGE imgZM[22];
IMAGE imgZMDead[20];
IMAGE imgZMEat[21];
IMAGE imgZMStand[11];

//子弹的数据类型
struct bullet {
    int x, y;
    int row;
    bool used;
    int speed;
    bool blast;
    int frameIndex;
};
struct bullet bullets[30];
IMAGE imgBulletNormal;
IMAGE imgBullBlast[4];

//检查文件是否存在	
bool fileExist(const char* name) {
    FILE* fp=fopen(name, "r");
	if (fp == NULL) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}

//初始化函数
void gameInit() {
	//加载背景图片
	loadimage(&imgBg, "res/bg.jpg");
	loadimage(&imgBar5, "res/bar5.png");
	memset(imgZhiWu, 0, sizeof(imgZhiWu));//把一块内存区域的内容全部设置为0
	memset(map, 0, sizeof(map));

	//初始化植物卡牌
	char name[64];//存放图片的名字，用64，2的几次幂存储内存
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		//生成植物卡牌的图片名字
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);
		loadimage(&imgCards[i], name);
		for (int j = 0; j < 20; j++) {
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i,j + 1);
			//先判断这个文件是否存在
			if (fileExist(name)) {
				imgZhiWu[i][j] = new IMAGE;//用c++的方式，创建一个新的图片
				loadimage(imgZhiWu[i][j], name);
			}
			else 
				break;
		}
	}
	curZhiWu = 0;
	//初始化植物
	memset(balls, 0, sizeof(balls));//把阳光池的内容全部设置为0
	//加载阳光图片
	for (int i = 0; i < 29; i++) {
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSunshineBall[i], name);
	}
	
	//配置随机种子
	srand(time(NULL));
	//创建游戏图形窗口（窗口的宽度和高度）
	initgraph(WIN_WIDTH,WIN_HEIGHT, 1);//定义为宏，方便修改，其中的1表示还会弹出一个窗口（控制台窗口）

	// 设置字体（用于显示阳光数量）
	LOGFONT f;
	gettextstyle(&f);//获取当前字体
	f.lfHeight = 30;//设置字体大小
	f.lfWeight = 15;//设置字体粗细
	strcpy_s(f.lfFaceName, "Segoe UI Black");//设置字体
	f.lfQuality = ANTIALIASED_QUALITY; // 抗锯齿效果
	settextstyle(&f);
	setbkmode(TRANSPARENT);//设置背景透明
	setcolor(BLACK);//设置字体颜色

	// 初始化僵尸数据
	memset(zms, 0, sizeof(zms));
	for (int i = 0; i < 22; i++) {
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i+1);
		loadimage(&imgZM[i], name);
	}

	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");

	// 初始化子弹数据
	memset(bullets, 0, sizeof(bullets));
// 初始化豌豆子弹的帧图片数组
	loadimage(&imgBullBlast[3], "res/bullets/bullet_blast.png");
	for(int i = 0; i < 3; i++){
		float k = (i + 1) * 0.2;
		loadimage(&imgBullBlast[i], "res/bullets/bullet_blast.png",
		imgBullBlast[3].getwidth() * k,
		imgBullBlast[3].getheight() * k,
		true);
	}
	for(int i = 0;i < 20;i++){
		sprintf_s(name,sizeof(name),"res/zm_dead/%d.png",i + 1);
		loadimage(&imgZMDead[i],name);
	}
	for(int i = 0; i < 21; i++){
		sprintf_s(name,"res/zm_eat/%d.png",i + 1);
		loadimage(&imgZMEat[i],name);
	}
	for(int i=0;i<11;i++){
		sprintf_s(name,sizeof(name),"res/zm_stand/%d.png",i + 1);
		loadimage(&imgZMStand[i],name);
	}
	loadimage(&imgEnd,"res/gameFail2.png");
	loadimage(&imgEnd1,"res/gameWin2.png");
}

void drawBullets() {
int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
for (int i = 0; i < bulletMax; i++) {
    if (bullets[i].used) {
	    if (bullets[i].blast) {
            IMAGE* img = &imgBullBlast[bullets[i].frameIndex];
            putimagePNG(bullets[i].x, bullets[i].y, img);
        } else {
        putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
		}
	}
}
EndBatchDraw();//结束双缓冲
}

void collectSunshine(ExMessage* msg) {
	int count = sizeof(balls) / sizeof(balls[0]);
	int w = imgSunshineBall[0].getwidth();
	int h = imgSunshineBall[0].getheight();
	for (int i = 0; i < count; i++) {
		if (balls[i].used) {
			int x = balls[i].pCur.x;
			int y = balls[i].pCur.y;
			if (msg->x > x && msg->x < x + w &&
				msg->y > y && msg->y < y + h) {
				//balls[i].used = false;
				balls[i].status = SUNSHINE_COLLECT;
				//sunshine += 25;
				//PlaySound("res/sunshine1.wav", NULL, SND_FILENAME);
				balls[i].p1 = balls[i].pCur;
				balls[i].p4 = vector2(262, 0);
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);
				float off = 8;
				balls[i].speed = 1.0 / (distance / off);
				break;
			}
		}
	}
}

void userClick(){
	ExMessage msg;//一个结构体，用来存储消息
	static int status = 0;//状态机，0表示鼠标没有选中植物，1表示鼠标选中植物
	static int cnm = 0;
	if (peekmessage(&msg)) {
		if (msg.message == WM_LBUTTONDOWN) {//消息的类型是鼠标左键按下
			if (msg.x > 338 && msg.x < 338 + 64 * ZHI_WU_COUNT && msg.y < 96) {
				int index = (msg.x - 338) / 65;//计算选中的植物的索引（理解为框中的第几个植物）
				//printf("%d\n", index);//在控制台，打印选中的植物的索引，方便调试
				status = 1;//已经选中，状态机变为1
				curZhiWu = index + 1;
				if(curZhiWu==1)	
					if (sunshine >= 100)	sunshine -= 100;
					else
					{
						status = 0;//已经选中，状态机变为1
						curZhiWu = 0;
					}
				if (curZhiWu == 2)
					if(sunshine >=50)	sunshine -= 50;
					else
					{
						status = 0;//已经选中，状态机变为1
						curZhiWu = 0;
					}//当前选中的植物的索引，从0开始，所以要加1
				}
				else{
					collectSunshine(&msg);
				}
		}else if(msg.message == WM_MOUSEMOVE &&status == 1){
			curX = msg.x;
			curY = msg.y;
		}else if(msg.message == WM_LBUTTONUP&&status==1){
			if (msg.x > 180&& msg.y > 179 && msg.y < 489) {
				//计算鼠标点击的位置在哪个格子，然后放置植物在该格子中
				int row = (msg.y - 179) / 102;
				int col = (msg.x - 160) / 81;
				//printf("%d,%d\n", row, col);可以在控制台打印出鼠标点击的位置，方便调试
				//下面进行植物的放置
				if (map[row][col].type == 0) {
					map[row][col].type = curZhiWu;
					map[row][col].frameIndex = 0;
					map[row][col].x = 155+ col * 81;
					map[row][col].y = 179 + row * 102 + 14;
				}
			}
			curZhiWu = 0;//清空当前选中的植物，使鼠标处没有选中植物
			status = 0;
		}
	}
}

void drawZM() {
    int zmCount = sizeof(zms) / sizeof(zms[0]);
    // 遍历每个僵尸
    for (int i = 0; i < zmCount; i++) {
        if (zms[i].used) {
            // 获取僵尸对应的图像指针，imgZM是图像数组，frameIndex是图像帧索引
            //IMAGE* img = &imgZM[zms[i].frameIndex];
			//IMAGE *img = (zms[i].dead) ? imgZMDead : imgZM;
			IMAGE* img = NULL;
			if(zms[i].dead) img=imgZMDead;
			else if(zms[i].eating) img=imgZMEat;
			else img=imgZM;
			img += zms[i].frameIndex;
            // 在指定位置绘制图像，x和y是僵尸的坐标，img->getheight()获取图像高度
            putimagePNG(zms[i].x, zms[i].y-img->getheight(),img);
        }
    }
}

void drawSunshines(){
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		if(balls[i].used) {
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}
	}
}

void drawCards(){
 	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		int x = 338 + i * 65;
		int y = 6;
		putimage(x, y, &imgCards[i]);
	}
}
//拖动植物函数
void drawZhiWu(){
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				// int x = 256 + j * 81;
				// int y = 179 + i * 102+14；
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				putimagePNG(map[i][j].x,map[i][j].y, imgZhiWu[zhiWuType][index]);
			}
		}
	}
	//注意前后的顺序，先渲染植物，再渲染拖动过程中的植物，这样可以使拖动的植物在最上层
	//渲染拖动过程中的植物
	if (curZhiWu > 0) {
		IMAGE* img = imgZhiWu[curZhiWu - 1][0];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, img);//用png版本的图片，同时修改渲染的位置，使其处于鼠标的中心位置
	}
}

void updateWindow() {
	BeginBatchDraw();//开始缓冲，先打印到一段内存中，然后一次性输出到屏幕上
	putimage(-112, 0, &imgBg);
	putimagePNG(250, 0, &imgBar5);
	drawCards();
	drawZhiWu();	
	drawSunshines();
	drawZM();
	drawBullets();
	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshine);
	outtextxy(276, 67, scoreText);
	EndBatchDraw();//结束双缓冲
}

void creatSunshine() {
	static int fre = 400;
	//意味着第一次400帧产生一个阳光
	static int count = 0;
	count++;//计算调用次数
	if (count >= fre) {
		fre = 200 + rand() % 200;//随机时间（200-399）产生一个阳光
		count = 0;
		//从阳光池中找到一个未使用的阳光
		int ballMax = sizeof(balls) / sizeof(balls[0]);
		int i;
		for (i = 0; i < ballMax && balls[i].used; i++);
		if (i >= ballMax)	return;

		balls[i].used = true;
		balls[i].frameIndex = 0;
		balls[i].timer = 0;
		balls[i].status = SUNSHINE_DOWN;
		balls[i].t=0;
		balls[i].p1 = vector2(260 + rand() % (900 - 260), 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		int off = 2;
		float distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].speed = 1.0 / (distance / off);
	}
	int ballMax=sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == XIANG_RI_KUI + 1) {
				map[i][j].timer++;
				if (map[i][j].timer > 200) {
					map[i][j].timer = 0;
					int k;
					for (k = 0; k < ballMax && balls[k].used; k++);
					if (k >= ballMax) return;
					balls[k].used = true;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int w = (100 + rand() % 50) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x + w, map[i][j].y + imgZhiWu[XIANG_RI_KUI][0]->getheight() - imgSunshineBall[0].getheight());
					balls[k].p2 = vector2(balls[k].p1.x + w * 0.3, balls[k].p1.y - 100);
					balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 100);
					balls[k].status = SUNSHINE_PRODUCT;
					balls[k].t = 0;
					balls[k].speed = 0.05;
				}
			}
		}
	}
}

void updateSunshine() {
	int BallMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < BallMax; i++) {
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN) {
				struct sunshineBall* sun = &balls[i];
				balls[i].t += sun->speed;
				sun->pCur =sun->p1+(sun->p4 - sun->p1) * sun->t;
				if (sun->t >= 1) {
					sun->used = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND) {
				balls[i].timer++;
				if (balls[i].timer > 100) {
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_COLLECT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + (sun->p4 - sun->p1) * sun->t;
				if(sun->t >= 1) {
					sun->used = false;
					sunshine+=25;
				}
			}
			else if (balls[i].status == SUNSHINE_PRODUCT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = calcBezierPoint(sun->t,sun->p1,sun->p2,sun->p3,sun->p4);
				if(sun->t >1) {
					sun->status= SUNSHINE_GROUND;
					sun->timer=0;
				}
			}
		}
	}
}

void createZM() {
	static int zmFre = 300;//每300帧产生一个僵尸，可以修改这个值，调整僵尸产生的频率
	static int count = 0;//调用次数，相当于帧数
    count++;
    if (count > zmFre) {
        count = 0;
        zmFre = rand() % 200 + 300;//随机时间（300-499）产生一个僵尸
		int i;
		int zmMax = sizeof(zms) / sizeof(zms[0]);
		for (i = 0; i < zmMax && zms[i].used; i++);
		if (i < zmMax) {
			memset(&zms[i], 0, sizeof(zms[i]));
			zms[i].used = true;
			zms[i].x = WIN_WIDTH;
			zms[i].row = rand() % 3;
			zms[i].y = 172 + (1 + zms[i].row) * 100;
			zms[i].speed = 1; 
			zms[i].blood = 100;
			zms[i].dead = false;
		}else{
			printf_s("创建僵尸失败\n");
		}
	
	}
}

void updateZM() {
    int zmMax = sizeof(zms) / sizeof(zms[0]);
	static int count = 0;
    count++;
    if (count > 2) {
        count = 0;
    // 更新僵尸的位置
		for (int i = 0; i < zmMax; i++) {
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;
				if (zms[i].x < 100) {
					//printf_s("GAME OVER\n");
					zmd = -1;
					//MessageBox(NULL, "over", "over", 0); 
					//exit(0); 
				}
			}
		}
	}
	static int count2 = 0;
    count2++;
    if (count2 > 4) {//每4帧更新一次帧索引,即每4帧僵尸的动作更新一次，可以修改这个值，调整僵尸的动作速度
        count2 = 0;
		for (int i = 0; i < zmMax; i++) {
			if (zms[i].used) {
				if(zms[i].dead){
					zms[i].frameIndex++;
					if(zms[i].frameIndex >= 20){
						zms[i].used = false;
					}
				}
				else if(zms[i].eating){
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 21;
				}
				else{
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 22;
				}
			}
		}
	}
}

void shoot() {
	int lines[3] = { 0 };
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int dangerX = WIN_WIDTH - imgZM[0].getwidth();
	for (int i = 0; i < zmCount; i++) {
		if (zms[i].used && zms[i].x < dangerX)
			lines[zms[i].row] = 1;
	}
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == WAN_DOU + 1 && lines[i]) {
				static int count = 0;
				count++;
				if (count > 80) {
					count = 0;
					int k;
					for (k = 0; k < bulletMax && bullets[k].used; k++);//找到一个未使用的子弹
					if (k < bulletMax) {
						bullets[k].used = true;
						bullets[k].row = i;
						bullets[k].speed = 3;//子弹的速度
						bullets[k].blast = false;
						bullets[k].frameIndex = 0;
						int zWX = 135 + j * 81+10;
						int zWY = 175 + i * 102+15 ;
						bullets[k].x = zWX + imgZhiWu[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = zWY +10;
					}
				}
			}
		}
	}
}

void updateBullets() {
    int countMax = sizeof(bullets) / sizeof(bullets[0]);
    for (int i = 0; i < countMax; i++) {
        if (bullets[i].used) {
            bullets[i].x += bullets[i].speed;
            if (bullets[i].x > WIN_WIDTH) {
                bullets[i].used = false;
            }

			// 待实现子弹的碰撞检测
			if (bullets[i].blast) {
				bullets[i].frameIndex++;
				if (bullets[i].frameIndex >= 4) 
                bullets[i].used = false;
            }
        }
    }
}

void checkBullet2Zm() {
	int bCount = sizeof(bullets) / sizeof(bullets[0]);
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < bCount; i++) {
		if (bullets[i].used == false || bullets[i].blast) continue;
		for (int k = 0; k < zCount; k++) {
			if (zms[k].used == false) continue;
			int x1 = zms[k].x + 80;
			int x2 = zms[k].x + 110;
			int x = bullets[i].x;
			if (zms[k].dead == false && bullets[i].row == zms[k].row && x > x1 && x < x2) {
				zms[k].blood -= 10;//每个子弹减少5滴血，可以修改这个值，调整子弹的伤害
				bullets[i].blast = true;
				bullets[i].speed = 0;

				if (zms[k].blood <= 0) {
					zms[k].dead = true;
					zms[k].speed = 0;
					zms[k].frameIndex = 0;
					zmd++;
				}
				break;
			}
		}
	}
}

void checkZm2ZhiWu() {
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zCount; i++) {
		if (zms[i].dead) continue;

		int row = zms[i].row;
		for (int k = 0; k < 9; k++) {
			if (map[row][k].type == 0) continue;
			int zhiWuX = 145 + k * 81;
			int x1 = zhiWuX + 10;
			int x2 = zhiWuX + 60;
			int x3 = zms[i].x + 80;
			if (x3 > x1 && x3 < x2) {
				if (map[row][k].catched) {
					//zms[i].frameIndex++;
					map[row][k].deadTime++;
					//if(zms[i].frameIndex > 100){
					if (map[row][k].deadTime > 100) {
						map[row][k].deadTime = 0;
						map[row][k].type = 0;
						zms[i].eating = false;
						zms[i].frameIndex = 0;
						zms[i].speed = 1;
					}
				}
				else {
					map[row][k].catched = true;
					map[row][k].deadTime = 0;
					zms[i].eating = true;
					zms[i].speed = 0;
					zms[i].frameIndex = 0;
				}
			}
		}
	}
}

void collisionCheck() {
    checkBullet2Zm();//检查子弹和僵尸的碰撞
	checkZm2ZhiWu();//检查僵尸和植物的碰撞
}

void updateGame() {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				map[i][j].frameIndex++;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				if (imgZhiWu[zhiWuType][index] == NULL) {
					map[i][j].frameIndex = 0;
				}
			}
		}
	}
	//思想：先调用一个函数，在去实现这个函数，这样可以使代码更加清晰
	creatSunshine();//创建阳光状态函数
	updateSunshine();//更新阳光状态函数
	createZM();//创建僵尸
	updateZM();//更新僵尸
	shoot();//射击
	updateBullets();//更新子弹
	collisionCheck();//碰撞检测
}

//显示用户界面
void startUI() {
	IMAGE imgBg, imgMenu1, imgMenu2;
	loadimage(&imgBg, "res/menu.png");
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");
	int flag = 0;

	while (1) {
		BeginBatchDraw();//双缓冲

		putimage(0, 0, &imgBg);
		putimagePNG(474, 75,flag? &imgMenu2 :&imgMenu1);

		//获取用户操作
		ExMessage msg;
		if (peekmessage(&msg)) {
			if (msg.message == WM_LBUTTONDOWN &&
				msg.x>474 &&msg.x < 474+300 &&
				msg.y >75 &&msg.y <75 +140) {
				flag = 1;
			}
			else if (msg.message == WM_LBUTTONUP &&flag) {
				EndBatchDraw();
				break;//而后就进入游戏界面，下一步骤。
			}
		}

		EndBatchDraw();
	}
}

void viewScence(){
	int xMin = WIN_WIDTH-imgBg.getwidth();
	vector2 points[9] = { {550,80},{530,160},{630,170},{530,200},{515,270},{565,370},{605,340},{705,280},{690,340} };//僵尸的最初的站位
	int index[9];
	for(int i=0;i<9;i++){
		index[i]=rand()%11;
	}
	//最先
	int count =0;
	for(int x=0;x>=xMin;x-=2){
		BeginBatchDraw();
		putimage(x,0,&imgBg);
		count++;
		for(int k=0;k<9;k++){
			putimagePNG(points[k].x-xMin+x,points[k].y,&imgZMStand[index[k]]);
			if(count>=10){
				index[k]=(index[k]+1)%11;
			}
		}
        if(count>=10)	count=0;
		EndBatchDraw();
		Sleep(10);
	}

	for(int i=0;i<50;i++){
		BeginBatchDraw();
		putimage(xMin, 0, &imgBg);
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x, points[k].y, &imgZMStand[index[k]]);
			index[k] = (index[k] + 1) % 11;
		}
		EndBatchDraw();
		Sleep(100);
	}

		for(int x=xMin;x<=-112;x+=2){
			BeginBatchDraw();
			putimage(x,0,&imgBg);
			count++;
			for(int k=0;k<9;k++){
				putimagePNG(points[k].x-xMin+x,points[k].y,&imgZMStand[index[k]]);
				if(count>5){
					index[k]=(index[k]+1)%11;
				}
			}
			if(count>5)	count=0;
			EndBatchDraw();
			Sleep(7);
		}	
}

void barsDown(){
	int height = imgBar.getheight();
	for (int y = -height; y <= 0; y++){
		BeginBatchDraw();
		putimage(-112, 0, &imgBg);//前两个参数是坐标（距离上，左两个边框的距离），后一个是图片
		putimagePNG(250, y, &imgBar);
		for (int i = 0; i < ZHI_WU_COUNT; i++){
			int x = 338 + i * 65;
			putimage(x, y+6, &imgCards[i]);
		}
		EndBatchDraw();
		Sleep(10);
	}
}

int main(void) {
	gameInit();//初始化函数
	startUI();//启动菜单函数
	viewScence();//开头
	barsDown();
	
	int timer = 0;//计时器
	bool flag = true;
	while (zmd>-1&&zmd<4) {
		userClick();//用户操作处理函数
		timer += getDelay();//获取延迟时间，加上上一次的时间间隔
		if (timer > 20) {//如果时间间隔大于20ms，就更新游戏数据，相当于每秒50帧
			flag = true;
			timer = 0;
		}
		if (flag){
			flag = false;
			updateWindow();//渲染函数
			updateGame();//游戏数据改变函数
		}
	}
	
	if(zmd>=4)	putimage(0, 0, &imgEnd1);
		else putimage(0, 0, &imgEnd);
	system("pause");
	
	return 0;
}
