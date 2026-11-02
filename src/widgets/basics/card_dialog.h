#ifndef CARD_DIALOG_H
#define CARD_DIALOG_H

#include <QtWidgets>

/**
 * A dialog drawn as a card: rounded, an optional header (a color marker, the title and a
 * secondary text on the right), the content, and a row of buttons at the bottom.
 * Opens centered on the main window, or next to a point with placeNear(). Moved by dragging
 * its header (or any part that is not a control).
 * Styled by the application style sheet (card_* object names), with built-in fallback colors.
 */
class CardDialog : public QDialog
{
    Q_OBJECT
public:
    enum ButtonKind {
        NORMAL,
        PRIMARY,        // the main action: also the default button (Enter)
        DANGER,         // a main action that throws something away
        DANGER_OUTLINE, // a secondary action that throws something away (e.g. Remove)
    };

    /// No title: no header (e.g. a question, with its title in the content)
    explicit CardDialog(QString title, QString secondary = QString(), QWidget *parent = nullptr, QColor marker = QColor());

    /// The content
    QVBoxLayout* body() {return body_layout;}

    /// A button at the bottom, from left to right. Cancel-like buttons should come first.
    QPushButton* addButton(QString text, ButtonKind kind = NORMAL, int stretch = 1);

    /// A muted text (e.g. a path, an explanation)
    QLabel* addText(QString text, QString object_name = "card_muted");

    /// Show it not modal, so that it can be moved: GNOME keeps modal dialogs attached to their
    /// window ("attach modal dialogs") and does not move them. It stays above its window.
    void showMovable();

    /// Next to this point (global coordinates), on the screen, not over `avoid` (global, e.g. the
    /// old and new position of a waypoint)
    void placeNear(QPoint global_pos, QRect avoid = QRect());

    /// A question card: returns true for the action. E.g. ask(w, "Reset the checklist?", "All items ...", "Reset checklist", DANGER)
    static bool ask(QWidget* parent, QString question, QString detail, QString action,
                    ButtonKind kind = PRIMARY, QString cancel = "Cancel");

    /// A short message card with OK: good news in green, a problem in red
    static void inform(QWidget* parent, QString title, QString detail, bool ok = true);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QVBoxLayout* body_layout;
    QHBoxLayout* buttons_layout;
    bool placed;
    QPoint drag_offset;     // while moving it by hand (when the window system can't)
    bool dragging = false;
};

#endif // CARD_DIALOG_H
