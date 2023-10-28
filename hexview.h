#ifndef HEXVIEW_H
#define HEXVIEW_H

#include <QAbstractScrollArea>

#define ROW_SPACING     5   //行间距
#define ROW_CHAR_COUNT  76  //每行字符数

class QLineEdit;

class HexView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit HexView(QWidget *parent = nullptr);
    ~HexView();

    QByteArray &data() {return m_data;}
    void setData(const QByteArray &data);
    void setBusy(bool);

protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void wheelEvent(QWheelEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);

signals:
    void clicked(QPoint);

private slots:
    void onClicked(QPoint);
    void onEditingFinished();

private:
    void adjustScrollRange();
    QLineEdit *m_edit;
    QByteArray m_data;
    QPoint m_pos; //鼠标按压位置
    int m_lineHeight;
    int m_lineWidth;
    int m_currentColumn; //当前选中的数据列 0 - 15
    int m_currentRow;   //当前选中的数据行，从0开始 每16个数据增1
    bool m_editVisible;
};

#endif // HEXVIEW_H
