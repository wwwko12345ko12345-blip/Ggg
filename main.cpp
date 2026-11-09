// Army Men RTS - NEW GAME - PS2SDK only, no gsKit, no dmaKit
// 10 Units, Map 5000x3000, WayPoints 10 (2 paths), Army GREEN, Ants RED, Cockroach BROWN, Nursery GREY-WHITE
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <pad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAP_W 5000
#define MAP_H 3000
#define MAX_UNITS 600
#define SPAWN_INTERVAL 20.0f

int plastic = 999999999;
int electricity = 999999999;

typedef enum { T_WORKER, T_SOLDIER, T_COCKROACH, T_NURSERY, T_GTOWER, T_PILLBOX, T_BULLDOZER, T_BWPOST, T_BARRACKS, T_SNIPER, T_COUNT } UnitType;
typedef struct { char name[32]; int hp, maxHp; float speed, seeingRange; int damage; float maxRange, minRange; float fireInterval, aimDelay, turnRate; int costPlastic, costElectric; float buildTime; int isBuilding; int isEnemy; } UnitDef;

UnitDef defs[T_COUNT] = {
  {"Worker",    80,80,50,81,30,16,0,0.5f,0,120,0,0,10,0,1},
  {"Soldier",  160,160,50,81,60,16,0,0.5f,0,120,0,0,15,0,1},
  {"Cockroach",300,300,50,81,50,30,0,0.5f,0,120,0,0,0,0,1},
  {"Nursery", 3000,3000,0,73,0,0,0,0,0,0,0,0,0,1,1},
  {"G-Tower",  800,800,0,72,25,72,8,0.8f,0.3f,180,200,0,15,1,0},
  {"Pillbox",  800,800,0,60,30,60,0,0.1f,0.0f,360,300,200,15,1,0},
  {"Bulldozer",400,400,25,40,0,0,0,0,0,150,250,50,20,0,0},
  {"BWPost",   150,150,0,20,0,0,0,0,0,0,25,0,3,1,0},
  {"Barracks",1200,1200,0,50,0,0,0,0,0,0,400,0,30,1,0},
  {"Sniper",    80,80,20,90,100,80,10,2.5f,0.5f,360,250,100,10,0,0},
};

typedef struct { int active; UnitType type; float x, y; int hp; int wpIndex; int pathId; float fireTimer; } Unit;
Unit units[MAX_UNITS];
int unitCount=0, selected=-1, buildMenu=0, buildChoice=0, paused=0;
float camX=2500, camY=1500, spawnTimer=0;
float waypoints[10][2] = {{200,500},{1000,500},{1800,800},{2500,1200},{3200,1500},{200,2500},{1000,2500},{1800,2200},{2500,1800},{3200,1500}};

int spawnUnit(UnitType t,float x,float y,int pathId){
  if(unitCount>=MAX_UNITS) return -1;
  for(int i=0;i<MAX_UNITS;i++) if(!units[i].active){
    units[i].active=1; units[i].type=t; units[i].x=x; units[i].y=y;
    units[i].hp=defs[t].hp; units[i].wpIndex=(pathId==0?0:5); units[i].pathId=pathId; units[i].fireTimer=0;
    unitCount++; return i;
  }
  return -1;
}
float dist2(float x1,float y1,float x2,float y2){ float dx=x2-x1, dy=y2-y1; return sqrtf(dx*dx+dy*dy); }
int findTarget(int idx){
  Unit *u=&units[idx]; UnitDef *d=&defs[u->type];
  if(d->damage==0) return -1;
  int best=-1; float bd=1e9;
  for(int i=0;i<MAX_UNITS;i++) if(units[i].active && i!=idx){
    if(defs[units[i].type].isEnemy==d->isEnemy) continue;
    float dd=dist2(u->x,u->y,units[i].x,units[i].y);
    if(dd>=d->minRange && dd<=d->maxRange && dd<bd){bd=dd;best=i;}
  }
  return best;
}
void update(float dt){
  if(paused) return;
  spawnTimer+=dt;
  if(spawnTimer>=SPAWN_INTERVAL){
    spawnTimer=0;
    for(int k=0;k<3;k++) spawnUnit(T_WORKER,200,500,0);
    for(int k=0;k<2;k++) spawnUnit(T_SOLDIER,200,2500,1);
    spawnUnit(T_COCKROACH,200,1500,0);
  }
  for(int i=0;i<MAX_UNITS;i++) if(units[i].active){
    Unit *u=&units[i]; UnitDef *d=&defs[u->type];
    if(d->isBuilding) continue;
    if(d->isEnemy){
      int wp=u->wpIndex;
      if(wp>=0&&wp<10){
        float tx=waypoints[wp][0], ty=waypoints[wp][1];
        float dd=dist2(u->x,u->y,tx,ty);
        if(dd<20){
          if(u->pathId==0 && wp<4) u->wpIndex++;
          else if(u->pathId==1 && wp<9) u->wpIndex++;
        } else {
          float dx=tx-u->x, dy=ty-u->y, len=sqrtf(dx*dx+dy*dy);
          if(len>0){ u->x+=dx/len*d->speed*dt*0.5f; u->y+=dy/len*d->speed*dt*0.5f; }
        }
      }
    }
    if(d->damage>0){
      u->fireTimer-=dt;
      int tgt=findTarget(i);
      if(tgt!=-1 && u->fireTimer<=0){
        units[tgt].hp-=d->damage;
        u->fireTimer=d->fireInterval+d->aimDelay;
        if(units[tgt].hp<=0){ units[tgt].active=0; unitCount--; }
      }
    }
  }
  // BWPost rule: enemy within half range damages BWPost
  for(int i=0;i<MAX_UNITS;i++) if(units[i].active && units[i].type==T_BWPOST){
    for(int j=0;j<MAX_UNITS;j++) if(units[j].active && defs[units[j].type].isEnemy){
      float half=defs[units[j].type].maxRange/2.0f;
      if(dist2(units[i].x,units[i].y,units[j].x,units[j].y)<half){
        units[i].hp-=5;
        if(units[i].hp<=0){ units[i].active=0; unitCount--; }
        break;
      }
    }
  }
  // Pillbox/G-Tower aggro: enemies move toward them
  int hasAggro=0;
  for(int i=0;i<MAX_UNITS;i++) if(units[i].active && (units[i].type==T_PILLBOX||units[i].type==T_GTOWER)) hasAggro=1;
  if(hasAggro){
    for(int i=0;i<MAX_UNITS;i++) if(units[i].active && defs[units[i].type].isEnemy && units[i].type!=T_NURSERY){
      int best=-1; float bd=1e9;
      for(int j=0;j<MAX_UNITS;j++) if(units[j].active && (units[j].type==T_PILLBOX||units[j].type==T_GTOWER)){
        float dd=dist2(units[i].x,units[i].y,units[j].x,units[j].y);
        if(dd<bd){bd=dd;best=j;}
      }
      if(best!=-1 && bd>10){
        float dx=units[best].x-units[i].x, dy=units[best].y-units[i].y, len=sqrtf(dx*dx+dy*dy);
        if(len>0){ units[i].x+=dx/len*defs[units[i].type].speed*0.016f; units[i].y+=dy/len*defs[units[i].type].speed*0.016f; }
      }
    }
  }
}

int main(int argc, char *argv[]){
  SifInitRpc(0);
  padInit(0); padPortOpen(0,0,(u32*)0);
  memset(units,0,sizeof(units));
  spawnUnit(T_BULLDOZER,3200,1500,0);
  spawnUnit(T_NURSERY,200,1500,0);
  struct padButtonStatus buttons;
  u32 old_pad=0;
  while(1){
    padRead(0,0,&buttons);
    u32 paddata=0xffff ^ buttons.btns;
    int lx=buttons.ljoy_h-128, ly=buttons.ljoy_v-128;
    if(abs(lx)>20) camX+=lx*0.08f;
    if(abs(ly)>20) camY+=ly*0.08f;
    if(camX<320) camX=320; if(camX>MAP_W-320) camX=MAP_W-320;
    if(camY<240) camY=240; if(camY>MAP_H-240) camY=MAP_H-240;
    if((paddata & PAD_R1) && !(old_pad & PAD_R1)){
      for(int i=0;i<MAX_UNITS;i++){ int idx=(selected+1+i)%MAX_UNITS; if(units[idx].active && !defs[units[idx].type].isEnemy){ selected=idx; break; } }
    }
    if((paddata & PAD_CROSS) && !(old_pad & PAD_CROSS)){
      if(buildMenu){
        int hasBarracks=0; for(int i=0;i<MAX_UNITS;i++) if(units[i].active && units[i].type==T_BARRACKS) hasBarracks=1;
        int costsP[5]={400,300,200,25,250};
        int costsE[5]={0,200,0,0,100};
        UnitType types[5]={T_BARRACKS,T_PILLBOX,T_GTOWER,T_BWPOST,T_SNIPER};
        if(!(!hasBarracks && buildChoice!=0) && plastic>=costsP[buildChoice] && electricity>=costsE[buildChoice]){
          plastic-=costsP[buildChoice]; electricity-=costsE[buildChoice];
          spawnUnit(types[buildChoice], camX+40, camY+40, 0);
        }
        buildMenu=0;
      } else {
        int best=-1; float bd=1e9;
        for(int i=0;i<MAX_UNITS;i++) if(units[i].active && !defs[units[i].type].isEnemy){
          float dd=dist2(units[i].x,units[i].y,camX,camY); if(dd<bd){bd=dd;best=i;}
        }
        selected=best;
      }
    }
    if(((paddata & PAD_CIRCLE) && !(old_pad & PAD_CIRCLE)) || ((paddata & PAD_SQUARE) && !(old_pad & PAD_SQUARE))){
      if(buildMenu) buildMenu=0; else selected=-1;
    }
    if((paddata & PAD_START) && !(old_pad & PAD_START)){
      if(paused){
        memset(units,0,sizeof(units)); unitCount=0; selected=-1;
        plastic=999999999; electricity=999999999;
        spawnUnit(T_BULLDOZER,3200,1500,0);
        spawnUnit(T_NURSERY,200,1500,0);
        paused=0;
      } else paused=1;
    }
    if((paddata & PAD_L1) && !(old_pad & PAD_L1)){ buildMenu=!buildMenu; buildChoice=0; }
    if(buildMenu){
      if((paddata & PAD_UP) && !(old_pad & PAD_UP)){ if(buildChoice>0) buildChoice--; }
      if((paddata & PAD_DOWN) && !(old_pad & PAD_DOWN)){ if(buildChoice<4) buildChoice++; }
    }
    old_pad=paddata;
    update(0.016f);
  }
  return 0;
}
