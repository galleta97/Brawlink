#pragma once
#include <memory>
#include <vector>

class Scene {
public:
    virtual ~Scene() = default;
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void update(float dt) = 0;   // dt real; LevelScene aplica el tiempo bala
    virtual void draw() = 0;
};

class SceneManager {
public:
    void change(std::unique_ptr<Scene> s) { pending_ = std::move(s); replace_ = true; }
    void push(std::unique_ptr<Scene> s)   { pending_ = std::move(s); replace_ = false; } // pausa encima del nivel
    void pop() { popRequested_ = true; }
    Scene* top() { return stack_.empty() ? nullptr : stack_.back().get(); }
    void drawAll() { for (auto& s : stack_) s->draw(); }   // el nivel se ve detrás de la pausa
    void applyChanges() {
        if (popRequested_ && !stack_.empty()) { stack_.back()->onExit(); stack_.pop_back(); }
        popRequested_ = false;
        if (pending_) {
            if (replace_) while (!stack_.empty()) { stack_.back()->onExit(); stack_.pop_back(); }
            pending_->onEnter();
            stack_.push_back(std::move(pending_));
        }
    }
private:
    std::vector<std::unique_ptr<Scene>> stack_;
    std::unique_ptr<Scene> pending_;
    bool replace_ = false, popRequested_ = false;
};
