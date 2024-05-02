class point{

public:
    point(int x, int y) : x(x), y(y) {}
    int x;
    int y;
};

point createPoint()
{
    return point(1, 2);
}
int main()
{
    return 0;
}