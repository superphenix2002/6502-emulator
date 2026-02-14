#define MAX 1<<16
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
uint8_t acc;
uint8_t y;
uint8_t x;
uint8_t pcl;
uint8_t pch;
uint8_t sp;
uint8_t status;
uint8_t memory[MAX];
uint16_t pc;

//

//BCD addition
uint8_t BCD_add(uint8_t reg1,uint8_t reg2){
uint8_t carry_in_2;
uint8_t result;
uint8_t a1 = reg1 & 0x0F;
uint8_t b1 = reg2 & 0x0F;
uint8_t a2 = (reg1 >> 4) & 0x0F;
uint8_t b2 = (reg2 >> 4) & 0x0F;
uint8_t c1 = a1+b1;
if(c1 > 9){
c1 = (c1 & 0x0F) + 6;
carry_in_2	= c1 >> 4;
}
uint8_t c2 = a2+b2+carry_in_2;
if(c2 > 9){
c2 = (c2 & 0x0F) + 6;
}
result = c2;
result = result << 4;
result |= c1 & 0x0F;
return result;
}


//BCD subtraction
uint8_t BCD_sub(uint8_t reg1,uint8_t reg2){
uint8_t reg2_hi = reg2 >> 4;
uint8_t reg2_lo = reg2 & 0x0F;
reg2_hi = 9 - reg2_hi;
reg2_lo = 9 - reg2_lo;
uint16_t reg2_comp = reg2_hi;
reg2_comp = reg2_comp << 4;
reg2_comp |= reg2_lo;
//uint8_t res1 = BCD_add(reg1,reg2_comp);
uint8_t carry_in_2;
uint8_t result;
uint8_t a1 = reg1 & 0x0F;
uint8_t b1 = reg2_comp & 0x0F;
uint8_t a2 = (reg1 >> 4) & 0x0F;
uint8_t b2 = (reg2_comp >> 4) & 0x0F;
uint8_t c1 = a1+b1;
if(c1 > 9){
c1 = (c1 & 0x0F) + 6;
carry_in_2	= c1 >> 4;
}
uint8_t c2 = a2+b2+carry_in_2;
if(c2 > 9){
c2 = (c2 & 0x0F) + 6;
}
if((c2 >> 4)!=0x00){
c1 = c1 + 1;	
}
result = c2;
result = result << 4;
result |= c1 & 0x0F;
return result;	
}


//carry flag
void update_flag_c(uint16_t reg){
if((reg >> 8)!=0x0000){        //set c flag
status |= 0x01;
}
else{              //clear c flag
status &= !0x01;
}
}

//overflow flag ADC
void update_flag_v_ADC(uint8_t reg1,uint8_t reg2,uint8_t reg3){

uint8_t regi1_t = reg1 & 0x7F;       
uint8_t regi2_t = reg2 & 0x7F;
uint8_t regi3_t = reg3 & 0x7F; 
uint8_t sum = regi1_t + regi2_t + regi3_t ;
uint8_t carry_in = sum >> 7;
uint8_t regi1_t_t = reg1 >> 7;       
uint8_t regi2_t_t = reg2 >> 7;
uint8_t regi3_t_t = reg3 >> 7; 
uint8_t sum2 = regi1_t_t + regi2_t_t + regi3_t_t + carry_in ;                         
uint8_t carry_out = sum2 >> 1;
if(carry_in != carry_out) status |= 0x40;
else status &= !0x40;

}

//overflow flag SBC
void update_flag_v_SBC(uint8_t reg1,uint8_t reg2,uint8_t reg3){
uint8_t regi2_comp = (!reg2) + 0x01;
uint8_t regi3_comp = (!reg3) + 0x01;
uint8_t regi1_t = reg1 & 0x7F;       
uint8_t regi2_t = regi2_comp & 0x7F;
uint8_t regi3_t = regi3_comp & 0x7F; 
uint8_t sum = regi1_t + regi2_t + regi3_t ;
uint8_t carry_in = sum >> 7;
uint8_t regi1_t_t = reg1 >> 7;       
uint8_t regi2_t_t = regi2_comp >> 7;
uint8_t regi3_t_t = regi3_comp >> 7; 
uint8_t sum2 = regi1_t_t + regi2_t_t + regi3_t_t + carry_in ;                         
uint8_t carry_out = sum2 >> 1;
if(carry_in != carry_out) status |= 0x40;
else status &= !0x40;

}

//overflow flag CMP
void update_flag_v_CMP(uint8_t reg1,uint8_t reg2){
uint8_t result;
result = reg1-reg2;
if((reg1 >> 7)==0x00){       //reg1 is positive
if((reg2 >> 7)==0x01){       //reg2 is negative
if((result >> 7)==0x01){     //result is negative
status |= 0x40;
}
else{                         //result is positive
status &= !0x40;
}
}
else{                        //reg2 is positive
status &= !0x40;
}

}
else{                      //reg1 is negative
if((reg2 >> 7)==0x00){       //reg2 is positive
if((result >> 7)==0x00){     //result is positive
status |= 0x40;
}
else{                         //result is negative
status &= !0x40;
}
}
else{                      //reg2 is negative
status &= !0x40;
}

}

}






int execute(uint8_t instr){

//instr = memory[...];
if(instr == 0x00){        //BRK                      //other instructions

pc=pc+2;
memory[sp] = pc >> 8;
sp=sp-1;
memory[sp] = pc;
sp=sp-1;
memory[sp] = status;
memory[sp] |= 0x10;
sp=sp-1;
status |= 0x04;
pc = 0xFFFE;
pch = pc >> 8;
pcl = pc;

}
else if(instr == 0x20){          //JSR abs

uint16_t pc_temp = pc+3;
memory[sp] = pc_temp >> 8;
sp=sp-1;
memory[sp] = pc_temp;
sp=sp-1;
pch = memory[pc+2];
pcl = memory[pc+1];
pc = pch;
pc = pc << 8;
pc |= pcl;

}
else if(instr == 0x40){          //RTI

sp=sp+1;
status = memory[sp];
sp=sp+1;
pcl = memory[sp];
sp=sp+1;
pch= memory[sp];
pc = pch;
pc = pc << 8;
pc |= pcl;

}
else if(instr == 0x60){         //RTS

pch = memory[sp+2];
pcl = memory[sp+1];
sp=sp+2;
uint16_t pc_16 = pch;
pc_16 = pc_16 << 8;
pc_16 |= pcl;
pc_16=pc_16+1;
pcl = pc_16;
pch = pc_16 >> 8;
pc = pc_16;

}
else if(instr == 0x08){           //PHP

memory[sp]=status;
memory[sp] |= 0x10;
sp=sp-1;
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x28){           //PLP

sp=sp+1;
status=memory[sp];
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x48){           //PHA

memory[sp]=acc;
sp=sp-1;
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x68){           //PLA

sp=sp+1;
acc=memory[sp];
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x88){           //DEY

y=y-1;
if(y==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((y >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0xA8){           //TAY

y=acc;
if(y==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((y >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0xC8){           //INY

y=y+1;
if(y==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((y >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0xE8){           //INX

x=x+1;
if(x==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((x >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x18){           //CLC

status &= !0x01;
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x38){           //SEC

status |= 0x01;
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x58){           //CLI

status &= !0x04;
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x78){           //SEI

status |= 0x04;
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x98){           //TYA

acc=y;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0xB8){           //CLV

status &= !0x40;
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0xD8){           //CLD

status &= !0x08;
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0xF8){           //SED

status |= 0x08;
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x8A){           //TXA

acc = x;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0x9A){           //TXS

sp=x;
if(sp==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((sp >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0xAA){           //TAX

x=acc;
if(x==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((x >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0xBA){           //TSX

x = sp;
if(x==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((x >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0xCA){           //DEX

x=x-1;
if(x==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((x >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if(instr == 0xEA){           //NOP
//DO NOTHING
pc=pc+1;
pcl = pc;
pch = pc >> 8;

}
else if((instr & 0x1F) == 0x10){         //xxy10000 pattern (unconditional jump)
uint8_t xx = instr >> 6;
uint8_t y = (instr >> 5) & 0x01;
switch(xx){
case 0:    //negative
if((status >> 7)==y){
pc = pc + 2 + memory[++pc];
pcl = pc;
pch = pc >> 8;	
}
else{
pc = pc + 2;
pcl = pc;
pch = pc >> 8;	
}
break;
case 1:    //overflow
if(((status >> 6) & 0x01) == y){
pc = pc + 2 + memory[++pc];
pcl = pc;
pch = pc >> 8;	
}
else{
pc = pc + 2;
pcl = pc;
pch = pc >> 8;	
}
break;
case 2:   //carry
if((status & 0x01) == y){
pc = pc + 2 + memory[++pc];
pcl = pc;
pch = pc >> 8;	
}
else{
pc = pc + 2;
pcl = pc;
pch = pc >> 8;	
}
break;
case 3:    //zero
if(((status >> 1) & 0x01) == y){
pc = pc + 2 + memory[++pc];
pcl = pc;
pch = pc >> 8;	
}
else{
pc = pc + 2;
pcl = pc;
pch = pc >> 8;	
}
break;
default:
//do nothing
break;
}

}
else{                      //aaabbbcc
uint8_t aaa = (instr >> 5) & 0x07;
uint8_t bbb = (instr >> 2) & 0x07;
if((instr & 0x03)==0x01){          //cc=01
switch(aaa){
case 0:        //ORA
//
switch(bbb){
case 0:  //(zero page,x)
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = acc | mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:  //zero page
{
uint16_t mem_off = memory[pc+1];
uint8_t mem_t = memory[mem_off];
acc = acc | mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:  //immediate
{
uint8_t mem_t = memory[pc+1];
acc = acc | mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:  //absolute
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
uint8_t mem_t = memory[mem_off];
acc = acc | mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:  //(zero page),y
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + y;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = acc | mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 5:   //zero page,x
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_t = memory[mem_off];
acc = acc | mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 6:  //absolute,y
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + y;
uint8_t mem_t = memory[mem_off];
acc = acc | mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:   //absolute,x
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + x;
uint8_t mem_t = memory[mem_off];
acc = acc | mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;	
	
}
//
break;
case 1:     //AND
//
switch(bbb){
case 0:  //(zero page,x)
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = acc & mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:  //zero page
{
uint16_t mem_off = memory[pc+1];
uint8_t mem_t = memory[mem_off];
acc = acc & mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:  //immediate
{
uint8_t mem_t = memory[pc+1];
acc = acc & mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:  //absolute
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
uint8_t mem_t = memory[mem_off];
acc = acc & mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:  //(zero page),y
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + y;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = acc & mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 5:   //zero page,x
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_t = memory[mem_off];
acc = acc & mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 6:  //absolute,y
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + y;
uint8_t mem_t = memory[mem_off];
acc = acc & mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:   //absolute,x
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + x;
uint8_t mem_t = memory[mem_off];
acc = acc & mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;	
	
}

//
break;
case 2:    //EOR
//

switch(bbb){
case 0:  //(zero page,x)
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = acc ^ mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:  //zero page
{
uint16_t mem_off = memory[pc+1];
uint8_t mem_t = memory[mem_off];
acc = acc ^ mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:  //immediate
{
uint8_t mem_t = memory[pc+1];
acc = acc ^ mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:  //absolute
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
uint8_t mem_t = memory[mem_off];
acc = acc ^ mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:  //(zero page),y
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + y;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = acc ^ mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 5:   //zero page,x
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_t = memory[mem_off];
acc = acc ^ mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 6:  //absolute,y
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + y;
uint8_t mem_t = memory[mem_off];
acc = acc ^ mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:   //absolute,x
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + x;
uint8_t mem_t = memory[mem_off];
acc = acc ^ mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
//clear overflow
status &= !0x40;
pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;	
	
}

//
break;
case 3:  //ADC
//

if(((status >> 3) & 0x01) == 0x00){   //decimal mode off

switch(bbb){
case 0:  //(zero page,x)
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = acc + mem_t + (status & 0x01);
uint16_t acc16 = acc + mem_t + (status & 0x01);
update_flag_v_ADC(acc,mem_t,status&0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:  //zero page
{
uint16_t mem_off = memory[pc+1];
uint8_t mem_t = memory[mem_off];
acc = acc + mem_t + (status & 0x01);
uint16_t acc16 = acc + mem_t + (status & 0x01);
update_flag_v_ADC(acc,mem_t,status&0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:  //immediate
{
uint8_t mem_t = memory[pc+1];
acc = acc + mem_t + (status & 0x01);
uint16_t acc16 = acc + mem_t + (status & 0x01);
update_flag_v_ADC(acc,mem_t,status&0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:  //absolute
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
uint8_t mem_t = memory[mem_off];
acc = acc + mem_t + (status & 0x01);
uint16_t acc16 = acc + mem_t + (status & 0x01);
update_flag_v_ADC(acc,mem_t,status&0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:  //(zero page),y
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + y;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = acc + mem_t + (status & 0x01);
uint16_t acc16 = acc + mem_t + (status & 0x01);
update_flag_v_ADC(acc,mem_t,status&0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 5:   //zero page,x
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_t = memory[mem_off];
acc = acc + mem_t + (status & 0x01);
uint16_t acc16 = acc + mem_t + (status & 0x01);
update_flag_v_ADC(acc,mem_t,status&0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 6:  //absolute,y
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + y;
uint8_t mem_t = memory[mem_off];
acc = acc + mem_t + (status & 0x01);
uint16_t acc16 = acc + mem_t + (status & 0x01);
update_flag_v_ADC(acc,mem_t,status&0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:   //absolute,x
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + x;
uint8_t mem_t = memory[mem_off];
acc = acc + mem_t + (status & 0x01);
uint16_t acc16 = acc + mem_t + (status & 0x01);
update_flag_v_ADC(acc,mem_t,status&0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;	
	
}

}
else{         //decimal mode on
switch(bbb){
case 0:  //(zero page,x)
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
//acc = acc + mem_t + (status & 0x01);
uint8_t res1 = BCD_add(mem_t,status & 0x01);
acc = BCD_add(acc,res1);

if(acc > 0x99){      //set carry flag
status |= 0x01;	
}
else{     //clear carry flag
status &= !0x01;
}

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:  //zero page
{
uint16_t mem_off = memory[pc+1];
uint8_t mem_t = memory[mem_off];
//acc = acc + mem_t + (status & 0x01);
uint8_t res1 = BCD_add(mem_t,status & 0x01);
acc = BCD_add(acc,res1);

if(acc > 0x99){      //set carry flag
status |= 0x01;	
}
else{     //clear carry flag
status &= !0x01;
}

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:  //immediate
{
uint8_t mem_t = memory[pc+1];
//acc = acc + mem_t + (status & 0x01);
uint8_t res1 = BCD_add(mem_t,status & 0x01);
acc = BCD_add(acc,res1);

if(acc > 0x99){      //set carry flag
status |= 0x01;	
}
else{     //clear carry flag
status &= !0x01;
}

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:  //absolute
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
uint8_t mem_t = memory[mem_off];
//acc = acc + mem_t + (status & 0x01);
uint8_t res1 = BCD_add(mem_t,status & 0x01);
acc = BCD_add(acc,res1);

if(acc > 0x99){      //set carry flag
status |= 0x01;	
}
else{     //clear carry flag
status &= !0x01;
}

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:  //(zero page),y
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + y;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
//acc = acc + mem_t + (status & 0x01);
uint8_t res1 = BCD_add(mem_t,status & 0x01);
acc = BCD_add(acc,res1);

if(acc > 0x99){      //set carry flag
status |= 0x01;	
}
else{     //clear carry flag
status &= !0x01;
}

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 5:   //zero page,x
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_t = memory[mem_off];
//acc = acc + mem_t + (status & 0x01);
uint8_t res1 = BCD_add(mem_t,status & 0x01);
acc = BCD_add(acc,res1);

if(acc > 0x99){      //set carry flag
status |= 0x01;	
}
else{     //clear carry flag
status &= !0x01;
}

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 6:  //absolute,y
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + y;
uint8_t mem_t = memory[mem_off];
//acc = acc + mem_t + (status & 0x01);
uint8_t res1 = BCD_add(mem_t,status & 0x01);
acc = BCD_add(acc,res1);

if(acc > 0x99){      //set carry flag
status |= 0x01;	
}
else{     //clear carry flag
status &= !0x01;
}

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:   //absolute,x
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + x;
uint8_t mem_t = memory[mem_off];
//acc = acc + mem_t + (status & 0x01);
uint8_t res1 = BCD_add(mem_t,status & 0x01);
acc = BCD_add(acc,res1);

if(acc > 0x99){      //set carry flag
status |= 0x01;	
}
else{     //clear carry flag
status &= !0x01;
}

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;	
	
}
}

//
break;
case 4:  //STA
//

switch(bbb){
case 0:  //(zero page,x)
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
memory[mem_add] = acc;


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:  //zero page
{
uint16_t mem_off = memory[pc+1];
memory[mem_off] = acc;


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:  //immediate
{
memory[pc+1] = acc;


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:  //absolute
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
memory[mem_off] = acc;


pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:  //(zero page),y
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + y;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
memory[mem_add] = acc;


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 5:   //zero page,x
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
memory[mem_off] = acc;


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 6:  //absolute,y
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + y;
memory[mem_off] = acc;


pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:   //absolute,x
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + x;
memory[mem_off] = acc;


pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;	
	
}

//
break;
case 5:  //LDA
//

switch(bbb){
case 0:  //(zero page,x)
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:  //zero page
{
uint16_t mem_off = memory[pc+1];
uint8_t mem_t = memory[mem_off];
acc = mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:  //immediate
{
uint8_t mem_t = memory[pc+1];
acc = mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:  //absolute
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
uint8_t mem_t = memory[mem_off];
acc = mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:  //(zero page),y
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + y;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 5:   //zero page,x
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_t = memory[mem_off];
acc = mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 6:  //absolute,y
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + y;
uint8_t mem_t = memory[mem_off];
acc = mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:   //absolute,x
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + x;
uint8_t mem_t = memory[mem_off];
acc = mem_t;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;	
	
}

//
break;
case 6:  //CMP
//

switch(bbb){
case 0:  //(zero page,x)
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
uint8_t acc_temp = acc - mem_t;
uint16_t acc_temp16 = acc - mem_t;

update_flag_v_CMP(acc,mem_t);
update_flag_c(acc_temp16);

if(acc_temp==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc_temp >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:  //zero page
{
uint16_t mem_off = memory[pc+1];
uint8_t mem_t = memory[mem_off];
uint8_t acc_temp = acc - mem_t;
uint16_t acc_temp16 = acc - mem_t;

update_flag_v_CMP(acc,mem_t);
update_flag_c(acc_temp16);

if(acc_temp==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc_temp >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:  //immediate
{
uint8_t mem_t = memory[pc+1];
uint8_t acc_temp = acc - mem_t;
uint16_t acc_temp16 = acc - mem_t;

update_flag_v_CMP(acc,mem_t);
update_flag_c(acc_temp16);

if(acc_temp==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc_temp >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:  //absolute
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
uint8_t mem_t = memory[mem_off];
uint8_t acc_temp = acc - mem_t;
uint16_t acc_temp16 = acc - mem_t;

update_flag_v_CMP(acc,mem_t);
update_flag_c(acc_temp16);

if(acc_temp==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc_temp >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:  //(zero page),y
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + y;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
uint8_t acc_temp = acc - mem_t;
uint16_t acc_temp16 = acc - mem_t;

update_flag_v_CMP(acc,mem_t);
update_flag_c(acc_temp16);

if(acc_temp==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc_temp >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 5:   //zero page,x
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_t = memory[mem_off];
uint8_t acc_temp = acc - mem_t;
uint16_t acc_temp16 = acc - mem_t;

update_flag_v_CMP(acc,mem_t);
update_flag_c(acc_temp16);

if(acc_temp==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc_temp >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 6:  //absolute,y
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + y;
uint8_t mem_t = memory[mem_off];
uint8_t acc_temp = acc - mem_t;
uint16_t acc_temp16 = acc - mem_t;

update_flag_v_CMP(acc,mem_t);
update_flag_c(acc_temp16);

if(acc_temp==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc_temp >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:   //absolute,x
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + x;
uint8_t mem_t = memory[mem_off];
uint8_t acc_temp = acc - mem_t;
uint16_t acc_temp16 = acc - mem_t;

update_flag_v_CMP(acc,mem_t);
update_flag_c(acc_temp16);

if(acc_temp==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc_temp >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;	
	
}

//
break;
case 7:   //SBC
//
if(((status >> 3) & 0x01) == 0x00){   //decimal mode off

switch(bbb){
case 0:  //(zero page,x)
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = acc - mem_t - ((!status) & 0x01);
uint16_t acc16 = acc + mem_t + (status & 0x01);
update_flag_v_SBC(acc,mem_t,(!status) & 0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:  //zero page
{
uint16_t mem_off = memory[pc+1];
uint8_t mem_t = memory[mem_off];
acc = acc - mem_t - ((!status) & 0x01);
uint16_t acc16 = acc - mem_t - ((!status) & 0x01);
update_flag_v_SBC(acc,mem_t,(!status) & 0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:  //immediate
{
uint8_t mem_t = memory[pc+1];
acc = acc - mem_t - ((!status) & 0x01);
uint16_t acc16 = acc - mem_t - ((!status) & 0x01);
update_flag_v_SBC(acc,mem_t,(!status) & 0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:  //absolute
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
uint8_t mem_t = memory[mem_off];
acc = acc - mem_t - ((!status) & 0x01);
uint16_t acc16 = acc - mem_t - ((!status) & 0x01);
update_flag_v_SBC(acc,mem_t,(!status) & 0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:  //(zero page),y
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + y;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
acc = acc - mem_t - ((!status) & 0x01);
uint16_t acc16 = acc - mem_t - ((!status) & 0x01);
update_flag_v_SBC(acc,mem_t,(!status) & 0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 5:   //zero page,x
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_t = memory[mem_off];
acc = acc - mem_t - ((!status) & 0x01);
uint16_t acc16 = acc - mem_t - ((!status) & 0x01);
update_flag_v_SBC(acc,mem_t,(!status) & 0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 6:  //absolute,y
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + y;
uint8_t mem_t = memory[mem_off];
acc = acc - mem_t - ((!status) & 0x01);
uint16_t acc16 = acc - mem_t - ((!status) & 0x01);
update_flag_v_SBC(acc,mem_t,(!status) & 0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:   //absolute,x
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + x;
uint8_t mem_t = memory[mem_off];
acc = acc - mem_t - ((!status) & 0x01);
uint16_t acc16 = acc - mem_t - ((!status) & 0x01);
update_flag_v_SBC(acc,mem_t,(!status) & 0x01);
update_flag_c(acc16);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;	
	
}

}
else{         //decimal mode on

switch(bbb){
case 0:  //(zero page,x)
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= (uint16_t)mem_add_lo;
uint8_t mem_t = memory[mem_add];
//acc = acc - mem_t - ((!status) & 0x01);
uint8_t res1 = BCD_sub(acc,mem_t);
acc = BCD_sub(res1,(!status) & 0x01);


if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x81;
}
else{   //positive
status &= !0x81;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:  //zero page
{
uint16_t mem_off = memory[pc+1];
uint8_t mem_t = memory[mem_off];
//acc = acc - mem_t - ((!status) & 0x01);
uint8_t res1 = BCD_sub(acc,mem_t);
acc = BCD_sub(res1,(!status) & 0x01);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:  //immediate
{
uint8_t mem_t = memory[pc+1];
//acc = acc - mem_t - ((!status) & 0x01);
uint8_t res1 = BCD_sub(acc,mem_t);
acc = BCD_sub(res1,(!status) & 0x01);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:  //absolute
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= mem_off_lo;
uint8_t mem_t = memory[mem_off];
//acc = acc - mem_t - ((!status) & 0x01);
uint8_t res1 = BCD_sub(acc,mem_t);
acc = BCD_sub(res1,(!status) & 0x01);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:  //(zero page),y
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + y;
mem_off = mem_off >> 8;
uint8_t mem_add_lo = memory[mem_off];
uint8_t mem_add_hi = memory[mem_off+1];
uint16_t mem_add = mem_add_hi;
mem_add = mem_add << 8;
mem_add |= mem_add_lo;
uint8_t mem_t = memory[mem_add];
//acc = acc - mem_t - ((!status) & 0x01);
uint8_t res1 = BCD_sub(acc,mem_t);
acc = BCD_sub(res1,(!status) & 0x01);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 5:   //zero page,x
{
uint16_t mem_off = memory[pc+1];
mem_off = mem_off + x;
mem_off = mem_off >> 8;
uint8_t mem_t = memory[mem_off];
//acc = acc - mem_t - ((!status) & 0x01);
uint8_t res1 = BCD_sub(acc,mem_t);
acc = BCD_sub(res1,(!status) & 0x01);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 6:  //absolute,y
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + y;
uint8_t mem_t = memory[mem_off];
//acc = acc - mem_t - ((!status) & 0x01);
uint8_t res1 = BCD_sub(acc,mem_t);
acc = BCD_sub(res1,(!status) & 0x01);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:   //absolute,x
{
uint8_t mem_off_lo = memory[pc+1];
uint8_t mem_off_hi = memory[pc+2];
uint16_t mem_off = mem_off_hi;
mem_off = mem_off << 8;
mem_off |= (uint16_t)mem_off_lo;
mem_off = mem_off + x;
uint8_t mem_t = memory[mem_off];
//acc = acc - mem_t - ((!status) & 0x01);
uint8_t res1 = BCD_sub(acc,mem_t);
acc = BCD_sub(res1,(!status) & 0x01);

if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;	
	
}

}

//
break;
default:
//do nothing
break;
	
}

}
else if((instr & 0x03)==0x02){     //cc=10
switch(aaa){
case 0:           //ASL
//
switch(bbb){
case 0:   //immediate
{
uint8_t mem_t = memory[pc+1];
if((mem_t >> 7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
acc = mem_t << 1;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
if((memory[addr]>>7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] << 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:   //accumulator
{
if((acc>>7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
acc = acc << 1;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 1;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
if((memory[addr]>>7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] << 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:    //zero page,X
{
uint16_t addr = memory[pc+1];
addr = addr + x;
addr = addr >> 8;
if((memory[addr]>>7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] << 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:    //absolute,X
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
addr = addr + x;
if((memory[addr]>>7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] << 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;
}
//
break;
case 1:           //ROL
switch(bbb){
case 0:   //immediate
{
uint8_t mem_t = memory[pc+1];
uint8_t carry_old = status & 0x01;
if((mem_t >> 7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
acc = mem_t << 1;
if(carry_old == 0x01){         //old carry flag was 1
acc |= 0x01;
}
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
uint8_t carry_old = status & 0x01;
if((memory[addr]>>7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] << 1;
if(carry_old == 0x01){         //old carry flag was 1
memory[addr] |= 0x01;
}
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:   //accumulator
{
uint8_t carry_old = status & 0x01;
if((acc>>7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
acc = acc << 1;
if(carry_old == 0x01){         //old carry flag was 1
acc |= 0x01;
}
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 1;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
uint8_t carry_old = status & 0x01;
if((memory[addr]>>7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] << 1;
if(carry_old == 0x01){         //old carry flag was 1
memory[addr] |= 0x01;
}
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:    //zero page,X
{
uint16_t addr = memory[pc+1];
addr = addr + x;
addr = addr >> 8;
uint8_t carry_old = status & 0x01;
if((memory[addr]>>7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] << 1;
if(carry_old == 0x01){         //old carry flag was 1
memory[addr] |= 0x01;
}
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:    //absolute,X
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
addr = addr + x;
uint8_t carry_old = status & 0x01;
if((memory[addr]>>7)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] << 1;
if(carry_old == 0x01){         //old carry flag was 1
memory[addr] |= 0x01;
}
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;
}
break;
case 2:           //LSR
switch(bbb){
case 0:   //immediate
{
uint8_t mem_t = memory[pc+1];
if((mem_t & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
acc = mem_t >> 1;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

//clear n flag
status &= !0x80;

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
if((memory[addr] & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] >> 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

//clear n flag
status &= !0x80;


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:   //accumulator
{
if((acc & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
acc = acc >> 1;
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

//clear n flag
status &= !0x80;


pc = pc + 1;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
if((memory[addr] & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] >> 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

//clear n flag
status &= !0x80;


pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:    //zero page,X
{
uint16_t addr = memory[pc+1];
addr = addr + x;
addr = addr >> 8;
if((memory[addr] & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] >> 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

//clear n flag
status &= !0x80;


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:    //absolute,X
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
addr = addr + x;
if((memory[addr] & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] >> 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

//clear n flag
status &= !0x80;


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;
}
break;
case 3:           //ROR   
switch(bbb){
case 0:   //immediate
{
uint8_t mem_t = memory[pc+1];
uint8_t carry_old = status & 0x01;
if((mem_t & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
acc = mem_t >> 1;
if(carry_old == 0x01){         //old carry flag was 1
acc |= 0x80;
}
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
uint8_t carry_old = status & 0x01;
if((memory[addr] & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] >> 1;
if(carry_old == 0x01){         //old carry flag was 1
memory[addr] |= 0x80;
}
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:   //accumulator
{
uint8_t carry_old = status & 0x01;
if((acc & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
acc = acc >> 1;
if(carry_old == 0x01){         //old carry flag was 1
acc |= 0x80;
}
if(acc==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((acc >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 1;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
uint8_t carry_old = status & 0x01;
if((memory[addr] & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] >> 1;
if(carry_old == 0x01){         //old carry flag was 1
memory[addr] |= 0x80;
}
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:    //zero page,X
{
uint16_t addr = memory[pc+1];
addr = addr + x;
addr = addr >> 8;
uint8_t carry_old = status & 0x01;
if((memory[addr] & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] >> 1;
if(carry_old == 0x01){         //old carry flag was 1
memory[addr] |= 0x80;
}
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}


pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:    //absolute,X
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
addr = addr + x;
uint8_t carry_old = status & 0x01;
if((memory[addr] & 0x01)==0x01){   //set carry flag
status |= 0x01;
}
else{                        //clear carry flag
status &= !0x01;
}
memory[addr] = memory[addr] >> 1;
if(carry_old == 0x01){         //old carry flag was 1
memory[addr] |= 0x80;
}
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;
}
break;
case 4:           //STX
switch(bbb){
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
memory[addr] = x;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:   //accumulator
{
x = acc;
pc = pc + 1;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
memory[addr] = x;
pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:    //zero page,X
{
uint16_t addr = memory[pc+1];
addr = addr + x;
addr = addr >> 8;
memory[addr] = x;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:    //absolute,X
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
addr = addr + x;
memory[addr] = x;
pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;
}
break;
case 5:           //LDX
switch(bbb){
case 0:   //immediate
{
uint8_t mem_t = memory[pc+1];
x = mem_t;
if(x==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((x >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
x = memory[addr];
if(x==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((x >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 2:   //accumulator
{
x = acc;
if(x==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((x >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 1;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
x = memory[addr];
if(x==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((x >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:    //zero page,X
{
uint16_t addr = memory[pc+1];
addr = addr + x;
addr = addr >> 8;
x = memory[addr];
if(x==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((x >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:    //absolute,X
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
addr = addr + x;
x = memory[addr];
if(x==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((x >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;
}
break;
case 6:           //DEC
//
switch(bbb){
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
memory[addr] = memory[addr] - 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
memory[addr] = memory[addr] - 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:    //zero page,X
{
uint16_t addr = memory[pc+1];
addr = addr + x;
addr = addr >> 8;
memory[addr] = memory[addr] - 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:    //absolute,X
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
addr = addr + x;
memory[addr] = memory[addr] - 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;
}
//
break;
case 7:           //INC
//
switch(bbb){
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
memory[addr] = memory[addr] + 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
memory[addr] = memory[addr] + 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 3;
pcl = pc;
pch = pc >> 8;
}
break;
case 4:    //zero page,X
{
uint16_t addr = memory[pc+1];
addr = addr + x;
addr = addr >> 8;
memory[addr] = memory[addr] + 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
case 7:    //absolute,X
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
addr = addr + x;
memory[addr] = memory[addr] + 1;
if(memory[addr]==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}

if((memory[addr] >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}

pc = pc + 2;
pcl = pc;
pch = pc >> 8;
}
break;
default:
//do nothing
break;
}
//
break;
default:
//do nothing
break;
}

}
else if((instr & 0x03)==0x00){      //cc=00
switch(aaa){
case 1:   //BIT
switch(bbb){
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
uint8_t temp_val = memory[addr] & acc;
if(temp_val==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
status = (status & 0x3F) | (memory[addr] & 0xC0);
pc = pc + 2;
pch = pc >> 8;
pcl = pc;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
uint8_t temp_val = memory[addr] & acc;
if(temp_val==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
status = (status & 0x3F) | (memory[addr] & 0xC0);
pc = pc + 3;
pch = pc >> 8;
pcl = pc;
}
break;
default:
//do nothing
break;
}
break;
case 2:   //JMP
switch(bbb){
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
pc = memory[addr];
pch = pc >> 8;
pcl = pc;
}
break;
default:
//do nothing
break;
}
break;
case 3:   //JMP_abs
switch(bbb){
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr_t = addr_hi;
addr_t = addr_t << 8;
addr_t |= (uint16_t)addr_lo;
uint16_t addr = memory[addr_t+1];
addr = addr << 8;
addr |= (uint16_t)memory[addr_t];
pc = memory[addr];
pch = pc >> 8;
pcl = pc;
}
break;
default:
//do nothing
break;
}
break;
case 4:   //STY
switch(bbb){
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
memory[addr] = y;
pc = pc + 2;
pch = pc >> 8;
pcl = pc;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
memory[addr] = y;
pc = pc + 3;
pch = pc >> 8;
pcl = pc;
}
break;
case 5:    //zero page, x
{
uint16_t addr = memory[pc+1];
addr = addr + x;
addr = addr >> 8;
memory[addr] = y;
pc = pc + 2;
pch = pc >> 8;
pcl = pc;
}
break;
default:
//do nothing
break;
}
break;
case 5:   //LDY
switch(bbb){
case 0:   //imm
{
uint8_t imm = memory[pc+1];
y = imm;
if(y==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((y >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc = pc + 2;
pch = pc >> 8;
pcl = pc;
}
break;
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
y = memory[addr];
if(y==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((y >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc = pc + 2;
pch = pc >> 8;
pcl = pc;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
y = memory[addr];
if(y==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((y >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc = pc + 3;
pch = pc >> 8;
pcl = pc;
}
break;
case 5:    //zero page,x
{
uint16_t addr = memory[pc+1];
addr = addr + x;
addr = addr >> 8;
y = memory[addr];
if(y==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((y >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc = pc + 2;
pch = pc >> 8;
pcl = pc;
}
break;
case 7:    //absolute,x
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
addr = addr + x;
y = memory[addr];
if(y==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((y >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
pc = pc + 3;
pch = pc >> 8;
pcl = pc;
}
break;
default:
//do nothing
break;
}
break;
case 6:   //CPY
switch(bbb){
case 0:   //imm
{
uint8_t imm = memory[pc+1];
uint8_t temp_val = y - imm;
if(temp_val==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((temp_val >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
if(y >= imm){       //set carry
status |= 0x01;
}
else{               //clear carry
status &= !0x01;
}
pc = pc + 2;
pch = pc >> 8;
pcl = pc;
}
break;
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
uint8_t temp_val = y - memory[addr];
if(temp_val==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((temp_val >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
if(y >= memory[addr]){       //set carry
status |= 0x01;
}
else{               //clear carry
status &= !0x01;
}
pc = pc + 2;
pch = pc >> 8;
pcl = pc;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
uint8_t temp_val = y - memory[addr];
if(temp_val==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((temp_val >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
if(y >= memory[addr]){       //set carry
status |= 0x01;
}
else{               //clear carry
status &= !0x01;
}
pc = pc + 3;
pch = pc >> 8;
pcl = pc;
}
break;
default:
//do nothing
break;
}
break;
case 7:    //CPX
switch(bbb){
case 0:   //imm
{
uint8_t imm = memory[pc+1];
uint8_t temp_val = x - imm;
if(temp_val==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((temp_val >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
if(x >= imm){       //set carry
status |= 0x01;
}
else{               //clear carry
status &= !0x01;
}
pc = pc + 2;
pch = pc >> 8;
pcl = pc;
}	
break;
case 1:    //zero page
{
uint16_t addr = memory[pc+1];
uint8_t temp_val = x - memory[addr];
if(temp_val==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((temp_val >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
if(x >= memory[addr]){       //set carry
status |= 0x01;
}
else{               //clear carry
status &= !0x01;
}
pc = pc + 2;
pch = pc >> 8;
pcl = pc;
}
break;
case 3:    //absolute
{
uint8_t addr_lo = memory[pc+1];
uint8_t addr_hi = memory[pc+2];
uint16_t addr = addr_hi;
addr = addr << 8;
addr |= (uint16_t)addr_lo;
uint8_t temp_val = x - memory[addr];
if(temp_val==0x00){    //zero
status |= 0x02;
}
else{       //non-zero
status &= !0x02;
}
if((temp_val >> 7)==0x01){   //negative
status |= 0x80;
}
else{   //positive
status &= !0x80;
}
if(x >= memory[addr]){       //set carry
status |= 0x01;
}
else{               //clear carry
status &= !0x01;
}
pc = pc + 3;
pch = pc >> 8;
pcl = pc;
}
break;
default:
//do nothing
break;
}
break;
default:
//do nothing
break;	
}
}


}
}






void read_image_file(FILE* file){ 
int c=0;
uint8_t* p = memory;
 while(fread(p,sizeof(uint8_t),1,file) == 1) {
 if(ferror(file)){
 printf("An error occurred during file reading!\n");
 perror("Details: ");
 exit(1);
 } 
 p++;
 c++;
 if(c >= MAX){
 return;
 }
 }
}


int read_image(const char* image_path)
{
FILE* file = fopen(image_path, "rb");
if (!file) {
perror("File not read!\n");
return 0;
}
read_image_file(file);
fclose(file);
return 1;
}

void clearInputBuffer() {
int c;
while ((c = getchar()) != '\n' && c != EOF);
}


int getCharInput(const char* prompt) {
int ch;
    
printf("%s", prompt);
fflush(stdout);
    
clearInputBuffer();  
ch = getchar();
clearInputBuffer();
    
return ch;
}


int main(int argc, char* argv[]){
	
if (argc < 2)
{ /* show usage string */
printf("6502 [bin-file] missing\n");
exit(2);
}
for (int j = 1; j < argc; ++j)
{
if (!read_image(argv[j]))
{
printf("failed to load image: %s\n", argv[j]);
exit(1);
}
}

uint8_t instr;
int running = 1;
int affirm;
uint16_t hex_code;

enum
{
PC_START = 0x0000
};

pc = PC_START;

while(running){
instr = memory[pc];
execute(instr);


printf("Value in register Accumulator = %x \n",acc);
printf("Value in register X = %x \n",x);
printf("Value in register Y = %x \n",y);
printf("Value in register PCL = %x \n",pcl);
printf("Value in register PCH = %x \n",pch);
printf("Value in register SP = %x \n",sp);
printf("Value in register Status = %x \n",status);	
printf("Value in register Program counter= %x \n",pc);

affirm = 99;
while((affirm == 99)||(affirm == 67)){
printf("Enter memory location to inspect in hex : ");
scanf("%d",&hex_code);
printf("\n Value at memory location %x is %x \n",hex_code,memory[hex_code]);
/*
else{
printf("Bad memory location\n");
}
*/
printf("Want to continue(c)?\n");
affirm = getCharInput("Enter choice: ");
}

}

return 0;
}