#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "raylib.h"
#include "raymath.h"

struct Tile {
  int x;
  int y;
};

using Tiles = std::vector<Tile>;

class Collum {
public:
  Collum(const size_t rows)
      : tiles_(rows) {
    if (tiles_.empty()) {
      throw std::logic_error("Wenigstens eine Reihe");
    }
  }

  [[nodiscard]] int GetHeight() const { return static_cast<int>(tiles_.size()); }

  Tile& operator[](const size_t row) {
    if (row >= tiles_.size()) {
      throw std::logic_error("Außerhalb des Indexes");
    }
    return tiles_.at(row);
  }

private:
  Tiles tiles_;
};

using Field = std::vector<Collum>;

class Grid {
public:
  Grid(const size_t seite)
      : Grid(seite, seite) { }

  Grid(const size_t collums, const size_t rows)
      : field_(Field(collums, Collum(rows))) {

    for (auto i { 0uz }; i < collums; i++) {
      for (auto j { 0uz }; j < rows; j++) {
        auto& positionDurchlauf = field_.at(i)[j];
        positionDurchlauf = Tile { static_cast<int>(i), static_cast<int>(j) };
      }
    }
  }

  [[nodiscard]] Vector2 TransformationPoint(
      const float x, const float y, const float height = 0.0f) const {
    return Vector2 { offset_.x + x * i_.x + y * j_.x, offset_.y + x * i_.y + y * j_.y - height };
  }

  Collum& operator[](const size_t collum) {
    if (collum >= field_.size()) {
      throw std::logic_error("Außerhalb des Indexes");
    }
    return field_.at(collum);
  }

  [[nodiscard]] float GetMapSize() const { return static_cast<float>(field_.size()); }

private:
  const float rautenWidth_ { 64 };
  const float rautenHeight_ { 32 };

  Vector2 offset_ { 0, 0 }; // für raylib, die Kamera übernimmt das Verschieben
  Vector2 i_ { rautenWidth_ / 2, rautenHeight_ / 2 }; // ein Schritt in x: rechts unten
  Vector2 j_ { -rautenWidth_ / 2, rautenHeight_ / 2 }; // ein Schritt in y: links unten

  Field field_;
};

class Player {
public:
  Player(Grid& grid)
      : grid_(grid) { }

  void Update() {
    const float dt { GetFrameTime() };
    Vector2 direction { };

    if (IsKeyDown(KEY_W)) { // hoch
      direction.x -= 1;
      direction.y -= 1;
    }
    if (IsKeyDown(KEY_S)) { // runter
      direction.x += 1;
      direction.y += 1;
    }
    if (IsKeyDown(KEY_A)) { // links
      direction.x -= 1;
      direction.y += 1;
    }
    if (IsKeyDown(KEY_D)) { // rechts
      direction.x += 1;
      direction.y -= 1;
    }

    if (direction.x == 0 && direction.y == 0) {
      return;
    }

    direction = Vector2Normalize(direction);
    coordinates_ = Vector2Add(coordinates_, Vector2Scale(direction, speed_ * dt)); // fps Sicherheit

    coordinates_.x = std::clamp(coordinates_.x, 0.0f, grid_.GetMapSize());
    coordinates_.y = std::clamp(coordinates_.y, 0.0f, grid_.GetMapSize());
  }

  [[nodiscard]] Vector2 GetCoordinates() const { return coordinates_; }

  [[nodiscard]] float GetHeight() const { return z_; }

private:
  Grid& grid_; // has-a

  Vector2 coordinates_ { 0.5f, 0.5f };
  float speed_ { 4.0f };
  float z_ { 40.0f };
};

class RayLibImplementation {
public:
  RayLibImplementation(Grid& grid, Player& player)
      : grid_(grid)
      , player_(player) { }

  void Draw() {

    InitWindow(1000, 1000, "Isometric");
    SetTargetFPS(60);

    camera_.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };

    while (!WindowShouldClose()) {
      player_.Update();
      UpdateCamera();

      BeginDrawing();
      ClearBackground(RAYWHITE);

      BeginMode2D(camera_);
      DrawTiles();
      DrawPlayer();
      EndMode2D();

      EndDrawing();
    }
    CloseWindow();
  }

private:
  void DrawTiles() {
    for (auto collum { 0uz }; collum < static_cast<size_t>(grid_.GetMapSize()); collum++) {
      for (auto rows { 0uz }; rows < static_cast<size_t>(grid_[collum].GetHeight()); rows++) {
        const auto& tile = grid_[collum][rows];
        const float x { static_cast<float>(tile.x) };
        const float y { static_cast<float>(tile.y) };

        const Vector2 top { grid_.TransformationPoint(x, y) };
        const Vector2 right { grid_.TransformationPoint(x + 1, y) };
        const Vector2 bottom { grid_.TransformationPoint(x + 1, y + 1) };
        const Vector2 left { grid_.TransformationPoint(x, y + 1) };

        Color color { GREEN };

        if ((tile.x + tile.y) % 2 != 0) {
          color = GRAY;
        }

        DrawTriangle(top, left, right, color);
        DrawTriangle(left, bottom, right, color);
      }
    }
  }

  void DrawPlayer() {
    const Vector2 coordinates { player_.GetCoordinates() };
    const Vector2 feet { grid_.TransformationPoint(coordinates.x, coordinates.y) };
    const Vector2 body {
      grid_.TransformationPoint(coordinates.x, coordinates.y, player_.GetHeight())
    };

    DrawEllipse(static_cast<int>(feet.x), static_cast<int>(feet.y), 5, 5, BLACK);
    DrawCircleV(body, 5, RED);
  }

  void UpdateCamera() {
    const Vector2 coordinates { player_.GetCoordinates() };
    camera_.target
        = grid_.TransformationPoint(coordinates.x, coordinates.y, player_.GetHeight());

    const float wheel { GetMouseWheelMove() };
    if (wheel != 0.0f) {
      camera_.zoom = std::clamp(camera_.zoom + wheel * 0.1f, 1.0f, 4.0f);
    }
  }

private:
  Grid& grid_; // has-a
  Player& player_;
  Camera2D camera_ { { 0.0f, 0.0f }, { 0.0f, 0.0f }, 0.0f, 2.0f }; // offset, target, rotation, zoom
};

int main() {
  Grid grid { 100 };
  Player player { grid };
  RayLibImplementation raylib { grid, player };

  raylib.Draw();
  return 0;
}