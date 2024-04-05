int f(int a,int b)
{
return a+b;
}

int main()
{
    int a=f(f(1,2),f(4,5));
    return 1;
}