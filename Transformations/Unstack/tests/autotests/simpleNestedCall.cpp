int f(int a){
    return 5;
}
int g(int a){
    return 4;
}
int main()
{
    int a=f(g(5));
    return 1;
}