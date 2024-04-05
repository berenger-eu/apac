int f(int a)
{
    return a;
}
int& g(int& a)
{
    return a;
}
int main()
{
    
    int a=1;
    a=f(f(a));
    g(a)++;
    f(g(a));
    int b=f(a++);
    int c= g(a)++;
    return 0;
}