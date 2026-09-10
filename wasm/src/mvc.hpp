#pragma once
// A tiny MVC framework. Not a real-world-scale one — just enough
// structure (Model / View / Controller, with a subscribe/notify loop)
// to be honest about the name.
#include <vector>
#include <functional>

namespace mvc {

// Model: owns state, notifies subscribers when it changes.
class Model {
public:
    using Listener = std::function<void()>;

    void subscribe(Listener listener) {
        listeners_.push_back(std::move(listener));
    }

protected:
    void notify() {
        for (auto& l : listeners_) l();
    }

private:
    std::vector<Listener> listeners_;
};

// View: reacts to model changes by (re)rendering.
class View {
public:
    virtual void render() = 0;
    virtual ~View() = default;
};

// Controller: wires a Model + View together and handles input events.
class Controller {
public:
    virtual ~Controller() = default;
};

} // namespace mvc
