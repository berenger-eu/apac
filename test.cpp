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
int fonction_test(int* a,int& b,int & bis,int *& c,int d)
{
	d=3;
	int& temp3=b;
	int& temp4=temp3;
	temp4+=4;
	int temp=b+(*a)-5;
	int temp2=6;
	return 0;
}
int fonction_test2(int &b)
{
	return 2;
}
int main() {
  int n=1;
  int m=3;
  int* r=&n;
	fonction_test(&n,m,r,4);
	fonction_test2(m);
	return 0;
}
