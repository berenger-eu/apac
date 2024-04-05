int f(int a,int b)
{
    return a+b;
}
int g(int a)
{
    return a;
}
int main()
{
    int b=4;
    int a=g(f(f(b,2),g(4)+f(7,8)-g(b))*4);
    return 1;
}