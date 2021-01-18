// 201701977 권유나
#include <stdio.h>
//#include <stdlib.h>

struct student {
	char*  name;
	int number;
};

void swap(struct student* arg1, struct student* arg2){

	struct student temp;

	temp.name = arg1->name;
	arg1->name = arg2 -> name;
	arg2->name = temp.name;

	temp.number = arg1->number;
	arg1->number = arg2->number;
	arg2->number = temp.number;
}


int main(){
		
	struct student me;
	struct student you;
	
//	me.name = (char *)malloc(3);
//	you.name = (char *)malloc(3);

//	char* a = me.name;
//	char* b = you.name;
	

	me.name = "KYN";
	you.name = "JBK";

	me.number = 201701977;
	you.number = 2;

	printf("myname : %s\nmynumber : %d",me.name,me.number);
	printf("\ntaname : %s\nclass : %d",you.name,you.number);

	printf("\n[before] myname : %s, taname : %s",me.name,you.name);
	printf("\n[before] mynum : %d, tanum : %d",me.number,you.number);

	printf("\nswap Function call!!");
	swap(&me, &you);

	printf("\n[after] myname : %s, taname : %s",me.name,you.name);
	printf("\n[after] mynum : %d, tanum : %d\n",me.number,you.number);

//	me.name = a;
//	you.name = b;

//	free(me.name);
//	free(you.name);

	return 0; 

}
