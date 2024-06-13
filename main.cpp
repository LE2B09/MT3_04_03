#include <Novice.h>
#include <imgui.h>
#include "Math/MathFunction.h"

static const int kWindowWidth = 1280;
static const int kWindowHeight = 720;

// 円錐の振り子の構造体
struct ConicalPendulum
{
	Vector3 anchor;             // アンカーポイント。固定された端の位置
	float length;               // 紐の長さ
	float halfApexAngle;        // 円錐の頂角の半分
	float angle;                // 現在の角度
	float angularVelocity;      // 角速度ω
};

// ボールの構造体
struct Ball
{
	Vector3 position;           // ボールの位置
	Vector3 velocity;           // ボールの速度
	Vector3 acceleration;       // ボールの加速度
	float mass;                 // ボールの質量
	float radius;               // ボールの半径
	unsigned int color;         // ボールの色
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

	// 円錐の振り子
	ConicalPendulum conicalPendulum{};
	conicalPendulum.anchor = { 0.0f, 1.0f, 0.0f };
	conicalPendulum.length = 0.8f;
	conicalPendulum.halfApexAngle = 0.7f;
	conicalPendulum.angle = 0.0f;
	conicalPendulum.angularVelocity = 0.0f;

	Ball ball{};
	ball.position = { 1.2f, 0.0f, 0.0f };
	ball.mass = 2.0f;
	ball.radius = 0.05f;
	ball.color = WHITE;

	MathFunction Func;

	Vector3 translate{};
	Vector3 rotate{};

	Vector3 cameraTranslate = { 0.0f, 1.9f, -6.49f };
	Vector3 cameraRotate = { 0.26f, 0.0f, 0.0f };

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

		ImGui::Begin("Control Window");
		if (ImGui::Button("Start"))
		{
			isActive = true; // 動きを開始
		}
		if (ImGui::Button("Reset"))
		{
			isActive = false; // 動きを停止
			conicalPendulum.angle = 0.0f; // 角度をリセット
		}
		ImGui::SliderFloat("Length", &conicalPendulum.length, 0.1f, 2.0f);					// 紐の長さを調整
		ImGui::SliderFloat("Half Apex Angle", &conicalPendulum.halfApexAngle, 0.1f, 1.5f);	// 頂角の半分を調整
		ImGui::End();

		// 各種行列の計算
		Matrix4x4 worldMatrix = Func.MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate, translate);
		Matrix4x4 cameraMatrix = Func.MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, cameraRotate, cameraTranslate);
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
			// 円錐振り子の角速度の計算
			conicalPendulum.angularVelocity = std::sqrt((9.8f * std::cos(conicalPendulum.halfApexAngle)) / (conicalPendulum.length * std::sin(conicalPendulum.halfApexAngle) * std::sin(conicalPendulum.halfApexAngle)));
			conicalPendulum.angle += conicalPendulum.angularVelocity * deltaTime;
		}

		// ボールの位置を計算
		float radius = std::sin(conicalPendulum.halfApexAngle) * conicalPendulum.length;
		float height = std::cos(conicalPendulum.halfApexAngle) * conicalPendulum.length;
		ball.position.x = conicalPendulum.anchor.x + std::cos(conicalPendulum.angle) * radius;
		ball.position.y = conicalPendulum.anchor.y - height;
		ball.position.z = conicalPendulum.anchor.z - std::sin(conicalPendulum.angle) * radius;

		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///

		// Gridを描画
		Func.DrawGrid(viewProjectionMatrix, viewportMatrix);

		// 円錐振り子の紐を描画
		Vector3 conicalPendulumBallPosition = ball.position;
		Vector3 screenConicalBallPosition = Func.Transform(conicalPendulumBallPosition, Func.Multiply(viewProjectionMatrix, viewportMatrix));
		Vector3 screenConicalAnchorPosition = Func.Transform(conicalPendulum.anchor, Func.Multiply(viewProjectionMatrix, viewportMatrix));
		Novice::DrawLine((int)screenConicalAnchorPosition.x, (int)screenConicalAnchorPosition.y, (int)screenConicalBallPosition.x, (int)screenConicalBallPosition.y, WHITE);

		// 円錐振り子の球体を描画
		Sphere conicalPendulumSphere = { conicalPendulumBallPosition, ball.radius };
		Func.DrawSphere(conicalPendulumSphere, viewProjectionMatrix, viewportMatrix, ball.color);

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
