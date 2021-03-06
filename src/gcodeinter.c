#include <string.h>
#include "fio.h"
#include "clib.h"
#include "cnc_misc.h"
#include "gcodeinter.h"
#include "cnc-controller.h"
#define MAX_F 100.0
#define MAX_A 1

#define RMS_SPEED_FACTOR 0.00970180395

float X_STEP_LENGTH = X_STEP_LENGTH_MM;
float Y_STEP_LENGTH = Y_STEP_LENGTH_MM;
float Z_STEP_LENGTH = Z_STEP_LENGTH_MM;

int32_t abs_mode = 1;
float curr_x = 0;
float curr_y = 0;
float curr_z = 0;
float offset_x = 0;
float offset_y = 0;
float offset_z = 0;

float curr_v = 0;

void gcodeJob()
{
	char buf[128];

    CNC_Home();
    CNC_CalZ();
    CNC_HomeSurface();
    
    fio_printf(1, "\rWelcome to GCode Shell\r\n");
    while (1) {
        fio_printf(1, ">");

        fio_read(0, buf, 127);
        ExcuteGCode(buf);

        fio_printf(1, "\x06");
    }
}

static void CheckExist(char* gcode,struct Exist *exist){
    exist->x = 0;
    exist->y = 0;
    exist->z = 0;
    exist->i = 0;
    exist->j = 0;
    exist->k = 0;
    exist->f = 0;
    exist->s = 0;

    for (int i=0; i<strlen(gcode); i++){
        switch (gcode[i]){
            case 'X':
                exist->x = 1;
                break;
            case 'Y':
                exist->y = 1;
                break;
            case 'Z':
                exist->z = 1;
                break;
            case 'I':
                exist->i = 1;
                break;
            case 'J':
                exist->j = 1;
                break;
            case 'K':
                exist->k = 1;
                break;
            case 'F':
                exist->f = 1;
                break;
            case 'S':
                exist->s = 1;
                break;
        }
    }
}

static void retriveParameters(char gcode[], struct Exist *exist, struct Vector *v1, struct Vector *v2, uint32_t *r, uint32_t *s){
    int t = strlen(gcode);
    char tmp;

    for (int i=strlen(gcode)-1; i>=1; i--){
        if ((gcode[i]<48 || gcode[i]>57) && gcode[i]!='.' && gcode[i]!='-'){
            tmp = gcode[t];
            gcode[t] = '\0';
            if(gcode[i] == 'F' && v1 != NULL)v1->f = atof(gcode+i+1);
            else if(gcode[i] == 'Z' && v1 != NULL)v1->z = atof(gcode+i+1);
            else if(gcode[i] == 'Y' && v1 != NULL)v1->y = atof(gcode+i+1);
            else if(gcode[i] == 'X' && v1 != NULL)v1->x = atof(gcode+i+1);
            else if(gcode[i] == 'I' && v2 != NULL)v2->x = atof(gcode+i+1);
            else if(gcode[i] == 'J' && v2 != NULL)v2->y = atof(gcode+i+1);
            else if(gcode[i] == 'K' && v2 != NULL)v2->z = atof(gcode+i+1);
            else if(gcode[i] == 'R' && r != NULL)*r = atof(gcode+i+1);
            else if(gcode[i] == 'S' && s != NULL)*s = atof(gcode+i+1);
            
            gcode[t] = tmp;
            t = i;
        }
    }

    CheckExist(gcode, exist);
    return;
}

void line_move(uint32_t gnum, struct Vector v, struct Exist *exist){
    struct Vector sv;
    if (abs_mode){
        sv.x = v.x - curr_x + offset_x;    
        sv.y = v.y - curr_y + offset_y;
        sv.z = v.z - curr_z + offset_z;
    }else{
        sv.x = v.x;    
        sv.y = v.y;
        sv.z = v.z;
    } 
    if(!exist->x)sv.x = 0;
    if(!exist->y)sv.y = 0;
    if(!exist->z)sv.z = 0;

    curr_x += sv.x;
    curr_y += sv.y;
    curr_z += sv.z;
    
    if(!gnum){
        v.f = MAX_F;
    }else{
        if(!exist->f)
            v.f = curr_v;
    }
    if(v.f != curr_v){
        if(v.f > 0){
        CNC_SetFeedrate((v.f / RMS_SPEED_FACTOR) / 60);
        curr_v = v.f;
        }
    }
    CNC_Move((int32_t)(sv.x/X_STEP_LENGTH), (int32_t)(sv.y/Y_STEP_LENGTH), (int32_t)(sv.z/Z_STEP_LENGTH));
}

static void M03(uint32_t speed, struct Exist *exist){
    if(exist->s){
        CNC_SetSpindleSpeed(speed);
    }
}

static void G10(struct Vector v, struct Exist *exist){
    //TODO: Check isn't correct
    if(exist->x){
        offset_x = v.x;
    }
    if(exist->y){
        offset_y = v.y;
    }
    if(exist->z){
        offset_z = v.z;
    }
    return;
}

static void G11(void){
    offset_x = 0;
    offset_y = 0;
    offset_z = 0;
    return;
}

static void G28(struct Vector v, struct Exist *exist){
    uint32_t tmp = abs_mode;
    abs_mode = 1;
    line_move(0, v, exist);
    CNC_HomeSurface();
    abs_mode = tmp;
    return;
}

static void G29(struct Vector v, struct Exist *exist){
    uint32_t tmp = abs_mode;
    abs_mode = 1;
    CNC_HomeSurface();
    line_move(0, v, exist);
    abs_mode = tmp;
    return;
}

static void G92(struct Vector v, struct Exist *exist){
    if(exist->x){
        offset_x = curr_x - v.x;
    }
    if(exist->y){
        offset_y = curr_y - v.y;
    }
    if(exist->z){
        offset_z = curr_z - v.z;
    }
    return;
}

/*
void G02(char gcode[], struct Exist *exist){
    struct XYZ_Vector v;
    float vi, vj, vk, R, i_pos, j_pos, x_pos, y_pos;   
    char tmp;
    for (int i=strlen(gcode)-1; i>=1; i--){
        if ((gcode[i]<48 || gcode[i]>57) && gcode[i]!='.'){
            tmp = gcode[t];
            gcode[t] = '\0';
            printf("%s ", gcode+i+1);
            if(gcode[i] == 'F')v.f = atof(gcode+i+1);
            else if(gcode[i] == 'Z')v.z = atof(gcode+i+1);
            else if(gcode[i] == 'Y')v.y = atof(gcode+i+1);
            else if(gcode[i] == 'X')v.x = atof(gcode+i+1);
            else if(gcode[i] == 'K')vk = atof(gcode+i+1);
            else if(gcode[i] == 'J')vj = atof(gcode+i+1);
            else if(gcode[i] == 'I')vi = atof(gcode+i+1);
            else if(gcode[i] == 'R')R = atof(gcode+i+1);
            gcode[t] = tmp;
            t = i;
        }
    }
    
    if (abs_mode){
        v.x = v.x - curr_x;    
        v.y = v.y - curr_y;
        v.z = v.z - curr_z;
    }
    
    if(!exist->x)v.x = 0;
    if(!exist->y)v.y = 0;
    if(!exist->z)v.z = 0; 
    if(!exist->i)vi = 0;
    if(!exist->j)vj = 0;
    if(!exist->k)vk = 0;
    if(!exist->f)v.f = 0;
    float circle_x, circle_y, circle_z;
    float e;
    float new_R = (R+e)/2;
    x_pos = (x_pos/2);
    if (circle_x == 0)
    for (float theta = theta1; theta<theta2; theta+=0.1)
        
}*/
uint32_t ExcuteGCode(char *gcode){
    struct Exist exist;
    struct Vector v1;
    uint32_t s;

    // G00 G01 G02 G03 G90 G91 G92 M02 M03 M04 M17 M18 
    if (strncmp(gcode, "G00", 3) == 0 || 
        strncmp(gcode, "G0 ", 3) == 0  ){ 

        retriveParameters(gcode, &exist, &v1, NULL, NULL, NULL);
        line_move(0, v1, &exist);
    }else if( strncmp(gcode, "G01", 3) == 0 ||
        strncmp(gcode, "G1", 2) == 0  ){

        retriveParameters(gcode, &exist, &v1, NULL, NULL, NULL);
        line_move(1, v1, &exist);
    }else if (strncmp(gcode, "G04", 3) == 0){

    }else if (strncmp(gcode, "G10", 3) == 0){
        retriveParameters(gcode, &exist, &v1, NULL, NULL, NULL);
        G10(v1, &exist);
    }else if (strncmp(gcode, "G11", 3) == 0){
        G11();
    }else if (strncmp(gcode, "G20", 3) == 0){
        X_STEP_LENGTH = X_STEP_LENGTH_INCH;
        Y_STEP_LENGTH = Y_STEP_LENGTH_INCH;
        Z_STEP_LENGTH = Z_STEP_LENGTH_INCH;
    }else if (strncmp(gcode, "G21", 3) == 0){
        X_STEP_LENGTH = X_STEP_LENGTH_MM;
        Y_STEP_LENGTH = Y_STEP_LENGTH_MM;
        Z_STEP_LENGTH = Z_STEP_LENGTH_MM;
    }else if (strncmp(gcode, "G28", 3) == 0){
        retriveParameters(gcode, &exist, &v1, NULL, NULL, NULL);
        G28(v1, &exist);
    }else if (strncmp(gcode, "G29", 3) == 0){
        retriveParameters(gcode, &exist, &v1, NULL, NULL, NULL);
        G29(v1, &exist);
    }else if (strncmp(gcode, "G90", 3) == 0){
        abs_mode = 1;
    }else if (strncmp(gcode, "G91", 3) == 0){
        abs_mode = 0;
    }else if (strncmp(gcode, "G92", 3) == 0){
        retriveParameters(gcode, &exist, &v1, NULL, NULL, NULL);
        G92(v1, &exist);
    }else if ((strncmp(gcode, "M02", 3) == 0) ||
              (strncmp(gcode, "M30", 3) == 0)){
        return 1;
    }else if (strncmp(gcode, "M03", 3) == 0){
        retriveParameters(gcode, &exist, NULL, NULL, NULL, &s);
        M03(s, &exist);
    }else if (strncmp(gcode, "M05", 3) == 0){
        CNC_SetSpindleSpeed(0);
    }else if (strncmp(gcode, "M17", 3) == 0){
        CNC_EnableStepper();
    }else if (strncmp(gcode, "M18", 3) == 0){
        CNC_DisableStepper();
    }
    return 0;
}


/*
int main(){

    char gcode[] = "G00 X123.1 Y231.2 F100";
    ExcuteGCode(gcode);
    return 0;
}*/
