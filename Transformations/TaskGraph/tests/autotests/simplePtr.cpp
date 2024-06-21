int main()
{
    int i,j;
    int*p;
    i=4;
    j=15;
    p=&i;
    (*p)++;
    i++;
    j++;
    p=&j;
    (*p)++;
    i++;
    int a;
    return 0;
}