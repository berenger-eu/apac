int main()
{
    int i;
    i=4;
    int j;
    j=i;
    /*
    for(int k=0;k<4;k++)
    {
        i++;
        j++;
        for(int l=0;l<4;l++)
        {
            i++;
            j++;
        }
        for(int m=0;m<4;m++)
        {
            ;
        }
        int a;
        a++;
    }*/
    i=41;
    j++;
    i=4;
    for (int k=0;k<4;k++)
    {
        i++;
        j++;
    }
    j=4;
    i=4+i+j;
    int a;
    a=i+j;
    return 0;
}