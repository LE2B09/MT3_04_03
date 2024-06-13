#include <Novice.h>
#include <imgui.h>
#include "Math/MathFunction.h"

static const int kWindowWidth = 1280;
static const int kWindowHeight = 720;

// 振り子の構造体
struct Pendulum
{
    Vector3 anchor;             // アンカーポイント。固定された端の位置
    float length;               // 紐の長さ
    float angle;                // 現在の角度
    float angularVelocity;      // 角速度ω
    float angularAcceleration;  // 角加速度
};

const char kWindowTitle[] = "提出用課題";

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

    // キー入力結果を受け取る箱
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    int prevMouseX = 0;
    int prevMouseY = 0;
    bool isDragging = false;

    // 動いているかどうかのフラグ
    bool isActive = false;

    // デルタタイム
    float deltaTime = 1.0f / 60.0f;

    Pendulum pendulum{};
    pendulum.anchor = { 0.0f, 1.0f, 0.0f };
    pendulum.length = 0.8f;
    pendulum.angle = 0.7f;
    pendulum.angularVelocity = 0.0f;
    pendulum.angularAcceleration = 0.0f;

    MathFunction Func;

    Vector3 translate{};
    Vector3 rotate{};

    Vector3 cameraTranslate = { 0.0f, 1.9f, -6.49f };
    Vector3 cameraRotate = { 0.26f, 0.0f, 0.0f };

    // 球体の情報
    Sphere p{ { pendulum.anchor.x + std::sin(pendulum.angle) * pendulum.length, pendulum.anchor.y - std::cos(pendulum.angle) * pendulum.length, pendulum.anchor.z }, 0.08f };

    // ウィンドウの×ボタンが押されるまでループ
    while (Novice::ProcessMessage() == 0)
    {
        // フレームの開始
        Novice::BeginFrame();

        // キー入力を受け取る
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        // マウス入力を取得
        POINT mousePosition;
        GetCursorPos(&mousePosition);

        ///
        /// ↓更新処理ここから
        ///

        // マウスドラッグによる回転制御
        if (Novice::IsPressMouse(1))
        {
            if (!isDragging)
            {
                isDragging = true;
                prevMouseX = mousePosition.x;
                prevMouseY = mousePosition.y;
            }
            else
            {
                int deltaX = mousePosition.x - prevMouseX;
                int deltaY = mousePosition.y - prevMouseY;
                rotate.y += deltaX * 0.01f; // 水平方向の回転
                rotate.x += deltaY * 0.01f; // 垂直方向の回転
                prevMouseX = mousePosition.x;
                prevMouseY = mousePosition.y;
            }
        }
        else
        {
            isDragging = false;
        }

        // マウスホイールで前後移動
        int wheel = Novice::GetWheel();
        if (wheel != 0)
        {
            cameraTranslate.z += wheel * 0.01f; // ホイールの回転方向に応じて前後移動
        }

        ImGui::Begin("Window");
        if (ImGui::Button("Start"))
        {
            isActive = true; // 動きを開始
        }
        if (ImGui::Button("Reset"))
        {
            isActive = false; // 動きを停止
            pendulum.angle = 0.7f; // 角度をリセット
            pendulum.angularVelocity = 0.0f; // 角速度をリセット
            pendulum.angularAcceleration = 0.0f; // 角加速度をリセット
        }
        ImGui::End();

        // 各種行列の計算
        Matrix4x4 worldMatrix = Func.MakeAffineMatrix({ 1.0f,1.0f,1.0f }, rotate, translate);
        Matrix4x4 cameraMatrix = Func.MakeAffineMatrix({ 1.0f,1.0f,1.0f }, cameraRotate, cameraTranslate);
        Matrix4x4 viewWorldMatrix = Func.Inverse(worldMatrix);
        Matrix4x4 viewCameraMatrix = Func.Inverse(cameraMatrix);
        // 透視投影行列を作成
        Matrix4x4 projectionMatrix = Func.MakePerspectiveFovMatrix(0.45f, float(kWindowWidth) / float(kWindowHeight), 0.1f, 100.0f);
        // ビュー座標変換行列を作成
        Matrix4x4 viewProjectionMatrix = Func.Multiply(viewWorldMatrix, Func.Multiply(viewCameraMatrix, projectionMatrix));
        // ViewportMatrixビューポート変換行列を作成
        Matrix4x4 viewportMatrix = Func.MakeViewportMatrix(0.0f, 0.0f, float(kWindowWidth), float(kWindowHeight), 0.0f, 1.0f);

        // 振り子の更新
        if (isActive)
        {
            pendulum.angularAcceleration = -(9.8f / pendulum.length) * std::sin(pendulum.angle);
            pendulum.angularVelocity += pendulum.angularAcceleration * deltaTime;
            pendulum.angle += pendulum.angularVelocity * deltaTime;
        }

        // pは振り子の先端の位置
        p.center = { pendulum.anchor.x + std::sin(pendulum.angle) * pendulum.length, pendulum.anchor.y - std::cos(pendulum.angle) * pendulum.length, pendulum.anchor.z };

        ///
        /// ↑更新処理ここまで
        ///

        ///
        /// ↓描画処理ここから
        ///

        // Gridを描画
        Func.DrawGrid(viewProjectionMatrix, viewportMatrix);

        // 紐を描画
        Vector3 screenAnchor = Func.Transform(pendulum.anchor, Func.Multiply(viewProjectionMatrix, viewportMatrix));
        Vector3 screenBallPosition = Func.Transform(p.center, Func.Multiply(viewProjectionMatrix, viewportMatrix));
        Novice::DrawLine((int)screenAnchor.x, (int)screenAnchor.y, (int)screenBallPosition.x, (int)screenBallPosition.y, WHITE); // 線の色を指定

        // Sphereを描画 (球体を描画)
        Func.DrawSphere(p, viewProjectionMatrix, viewportMatrix, WHITE);

        ///
        /// ↑描画処理ここまで
        ///

        // フレームの終了
        Novice::EndFrame();

        // ESCキーが押されたらループを抜ける
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0)
        {
            break;
        }
    }

    // ライブラリの終了
    Novice::Finalize();
    return 0;
}
