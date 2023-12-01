class MaClass{
	public:
		char c;
		int y;
		int z,wsd;
	int methode_test(int a,int b)
	{
		return 0;
	}
};
int fonction_test(int* a,int& b,int & bis,int *& c,int d,int e)
{
	d=3;
	int& temp3=b;
	int& temp4=temp3;
	temp4+=4;
	int temp=b+(*a)-5;
	int temp2=6;
	return 0;
}

int fonction_test3(int &b)
{
	b++;
	return 5;
}
int fonction_test2(int &b)
{
	fonction_test3(b);
	return 2;
}
void recurs(int& a,int &b,int &c,int &d)
{
	a++;
	recurs(b,c,a,d);
}
void cycle2(int&c,int&d);
void cycle1(int&a,int&b)
{
	a++;
	cycle2(a,b);
}
	void cycle2(int&c,int&d)
	{
		d++;
		cycle1(c,d);
	}

int main() {
  int n=1,m=3,iho;
  int* n2=&n;
	int* r=&n;
  int est_const=5;
  int est_const2=6;
	//fonction_test(n2,m,n,r,est_const,est_const2);
	int rec1=0;
	int rec2=0;
	int rec3=0;
	int rec4=5;
	recurs(rec1,rec2,rec3,rec4);
	//fonction_test2(m);
	return 0;
}
