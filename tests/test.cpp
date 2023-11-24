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
	
	return 5;
}
int fonction_test2(int &b)
{
	fonction_test3(b);
	return 2;
}
int main() {
  int n=1;
  int* n2=&n;
  int m=3;
  int* r=&n;
  int est_const=5;
  int est_const2=6;
	fonction_test(n2,m,n,r,est_const,est_const2);
	
	fonction_test2(m);
	return 0;
}
