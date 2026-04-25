struct Point {
    int x;
    int y;
};

int main(int *p, int n, int k) {
    int a[4];
    int x;
    int i;

    struct Point pt;

    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    a[3] = 40;

    pt.x = 5;
    pt.y = 7;

    if (n > 0)
        x = a[k] + pt.x;
    else
        x = *p - pt.y;

    i = 0;
    while (i < 3) {
        x = x + a[i];
        i = i + 1;
    }

    return x;
}
