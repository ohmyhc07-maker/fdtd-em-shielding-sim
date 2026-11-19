#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"

#define N 640
#define MAX_QUEUE_SIZE (N + 1)
#define E 0
#define H 1
#define DX_MM 2
#define DT_NS 0.006337717808765
#define S 0.95
#define MUR (-0.025641025641026)

double queue[2][MAX_QUEUE_SIZE];
int front[2], rear[2], count[2];
int material, thickness, begin, end, tick;
double frequency;
char names[3][64] = {"공기", "고무", "구리"};
const double A[3] = {1.0, 0.981092778172, 0.0};
const double B[3] = {0.95, 0.313673023211, 0.0};
Font font;
Color background = {14, 20, 32, 255};
Color panel = {25, 34, 51, 255};
Color slab = {57, 70, 92, 255};
Color electricColor = {65, 214, 186, 255};
Color magneticColor = {255, 184, 91, 255};

int isFull(int q) {
    return count[q] == MAX_QUEUE_SIZE - 1;
}
int isEmpty(int q) {
    return count[q] == 0;
}
void enqueue(int q, double data) {
    if (isFull(q)) {
        printf("오류: 원형 큐가 가득 찼습니다.\n");
        exit(1);
    }
    rear[q] = (rear[q] + 1) % MAX_QUEUE_SIZE;
    queue[q][rear[q]] = data;
    count[q]++;
}
double dequeue(int q) {
    if (isEmpty(q)) {
        printf("오류: 원형 큐가 비었습니다.\n");
        exit(1);
    }
    front[q] = (front[q] + 1) % MAX_QUEUE_SIZE;
    count[q]--;
    return queue[q][front[q]];
}
double readValue(int q, int i) {
    return queue[q][(front[q] + 1 + i) % MAX_QUEUE_SIZE];
}
void reset(void) {
    for (int q = 0; q < 2; q++) {
        front[q] = 0;
        rear[q] = 0;
        count[q] = 0;
    }
    for (int i = 0; i < N; i++) enqueue(E, 0);
    for (int i = 0; i < N - 1; i++) enqueue(H, 0);
    int width = thickness / DX_MM;
    begin = (N - width) / 2;
    end = begin + width;
    tick = 0;
}
void updateMagnetic(void) {
    double left = dequeue(E);
    enqueue(E, left);
    for (int i = 0; i < N - 1; i++) {
        double right = dequeue(E);
        enqueue(E, right);
        double old = dequeue(H);
        enqueue(H, old + S * (left - right));
        left = right;
    }
}
void updateElectric(void) {
    double old0 = dequeue(E);
    enqueue(E, 0);
    double left = dequeue(H);
    enqueue(H, left);
    double old1 = 0, new1 = 0, oldLast = 0, newLast = 0;
    for (int i = 1; i < N - 1; i++) {
        double right = dequeue(H);
        enqueue(H, right);
        double old = dequeue(E);
        int m = 0;
        if (i >= begin && i < end) m = material;
        double next = A[m] * old + B[m] * (left - right);
        if (i == 60) {
            double t = (tick + 1) * DT_NS;
            double u = (t - 6.0) / 1.5;
            next += 0.25 * sin(6.28318530718 * frequency * (t - 6.0)) * exp(-u * u);
        }
        enqueue(E, next);
        if (i == 1) {
            old1 = old;
            new1 = next;
        }
        if (i == N - 2) {
            oldLast = old;
            newLast = next;
        }
        left = right;
    }
    double oldRight = dequeue(E);
    enqueue(E, oldLast + MUR * (newLast - oldRight));
    for (int i = 0; i < N; i++) {
        double value = dequeue(E);
        if (i == 0) value = old1 + MUR * (new1 - old0);
        enqueue(E, value);
    }
    tick++;
}
void text(const char message[], int x, int y, int size, Color color) {
    Vector2 position = {x, y};
    DrawTextEx(font, message, position, size, 1, color);
}
float position(double i) {
    return 75 + i * 950 / (N - 1);
}
void graph(int q, int top, const char title[], Color color) {
    DrawRectangle(45, top, 1010, 190, panel);
    if (material != 0) {
        DrawRectangle(position(begin), top + 48,
                      position(end) - position(begin), 110, slab);
        text("물질", position(begin), top + 27, 16, LIGHTGRAY);
    }
    text(title, 65, top + 14, 24, color);
    DrawLine(75, top + 105, 1025, top + 105, GRAY);
    DrawLine(position(60), top + 48, position(60), top + 158, GRAY);
    text("발생점", position(60) - 20, top + 166, 16, LIGHTGRAY);
    text("0", 75, top + 166, 16, LIGHTGRAY);
    text(TextFormat("%.2f m", (N - 1) * DX_MM / 1000.0), 966, top + 166, 16, LIGHTGRAY);
    for (int i = 0; i < count[q] - 1; i++) {
        double offset = 0;
        if (q == H) offset = 0.5;
        Vector2 p1 = {position(i + offset), top + 105 - readValue(q, i) * 230};
        Vector2 p2 = {position(i + 1 + offset), top + 105 - readValue(q, i + 1) * 230};
        DrawLineEx(p1, p2, 2, color);
    }
}
double input(const char prompt[], double low, double high, int integer) {
    char line[128], extra;
    double value;
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) exit(1);
        if (sscanf(line, " %lf %c", &value, &extra) == 1 && isfinite(value)) {
            if (value >= low && value <= high && (!integer || floor(value) == value)) return value;
        }
        printf("범위에 맞는 숫자를 입력해 주세요.\n");
    }
}
int main(void) {
    printf("0 공기 / 1 고무 / 2 구리 \n");
    material = input("물질 번호 (0~2): ", 0, 2, 1);
    thickness = input("두께 (10~120 mm, 정수): ", 10, 120, 1);
    if (thickness % DX_MM != 0) thickness++;
    printf("적용 두께: %d mm\n", thickness);
    frequency = input("펄스 중심 주파수 (0.5~3 GHz): ", 0.5, 3, 0);
    printf("격자 2mm / 고무는 학습용 모형 / 구리는 완전도체 근사입니다.\n");
    printf("자기장은 비교를 위해 Z0 * H로 환산해 표시합니다.\n");
    if (!FileExists("korean.ttf")) {
        printf("같은 폴더에 korean.ttf를 넣고 실행해 주세요.\n");
        return 1;
    }
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(1100, 620, "EM Wave - Circular Queue");
    SetTargetFPS(60);
    const char letters[] =
        "전자기파 차폐 실험 공기 고무 구리 물질 두께 중심주파수 "
        "시간 전기장 자기장 크기환산 발생점 위치 재생 일시정지 종료 다시시작 "
        "SPACE R mm GHz ns 0123456789.():|/~+-";
    int length;
    int *points = LoadCodepoints(letters, &length);
    font = LoadFontEx("korean.ttf", 48, points, length);
    UnloadCodepoints(points);
    if (!IsFontValid(font)) {
        printf("글꼴을 불러오지 못했습니다.\n");
        CloseWindow();
        return 1;
    }
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    reset();
    int paused = 1, finished = 0;
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) {
            reset();
            paused = 1;
            finished = 0;
        }
        if (IsKeyPressed(KEY_SPACE) && !finished) paused = !paused;
        if (!paused) {
            for (int i = 0; i < 6; i++) {
                updateMagnetic();
                updateElectric();
                if (tick * DT_NS >= 24) {
                    paused = 1;
                    finished = 1;
                    break;
                }
            }
        }
        BeginDrawing();
        ClearBackground(background);
        text("전자기파 차폐", 45, 25, 32, RAYWHITE);
        text(TextFormat("%.2f ns", tick * DT_NS), 925, 35, 22, LIGHTGRAY);
        text(TextFormat("%s  |  %d mm  |  %.2f GHz", names[material], thickness, frequency),
             45, 80, 21, LIGHTGRAY);
        graph(E, 125, "전기장", electricColor);
        graph(H, 335, "자기장 (크기 환산)", magneticColor);
        text("SPACE 재생 / 일시정지     R 다시 시작", 45, 560, 21, LIGHTGRAY);
        if (finished) text("종료", 997, 560, 21, SKYBLUE);
        else if (paused) text("일시정지", 955, 560, 21, SKYBLUE);
        else text("재생", 997, 560, 21, SKYBLUE);
        EndDrawing();
    }
    UnloadFont(font);
    CloseWindow();
    return 0;
}