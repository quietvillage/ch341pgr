#include "hexview.h"

#include <QLineEdit>
#include <QFontDatabase>
#include <QScrollBar>
#include <QPainter>
#include <QResizeEvent>
#include <QRegularExpressionValidator>

HexView::HexView(QWidget *parent)
    : QAbstractScrollArea{parent}
    , m_currentColumn(0)
    , m_currentRow(0)
    , m_editVisible(false)
{
    int index = QFontDatabase::addApplicationFont(":SourceCodePro.otf");
    if (index != -1) {
        QStringList fontList(QFontDatabase::applicationFontFamilies(index));
        if(fontList.count() > 0) {
            QFont font(fontList.at(0));
            font.setPointSize(11);
            this->setFont(font);
        }
    }

    QFontMetrics fm(this->font());
    m_lineHeight = fm.height();
    m_lineWidth = fm.horizontalAdvance('X') * 76;

    m_edit = new QLineEdit(this);
    m_edit->setFixedWidth(fm.horizontalAdvance("XX") + 7);
    m_edit->setFixedHeight(m_lineHeight + 1);
    m_edit->setMaxLength(2);
    m_edit->setVisible(false);
    QRegularExpression reg("[0-9A-Fa-f]{0,2}");
    QValidator *validator = new QRegularExpressionValidator(reg, this);
    m_edit->setValidator(validator);

    connect(this, SIGNAL(clicked(QPoint)), this, SLOT(onClicked(QPoint)));
    connect(m_edit, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
}


void HexView::paintEvent(QPaintEvent *)
{
    QPainter painter(this->viewport());
    QString text, ascii;
    int i, pos, len, x, y, step,
        viewWidth = this->viewport()->width(),
        viewHeight = this->viewport()->height();

    painter.fillRect(0, 0, viewWidth, viewHeight, QBrush(Qt::white));

    len = m_data.length();
    if (!len) {return;}

    x = -this->horizontalScrollBar()->value();
    y = step = m_lineHeight + ROW_SPACING;
    i = this->verticalScrollBar()->value();
    i -= i % step;
    this->verticalScrollBar()->setValue(i); //使其值始终为step的整数倍
    i = (i / step) << 4;

    painter.drawLine(0, y + ROW_SPACING, viewWidth, y + ROW_SPACING);
    painter.drawText(x, y,
                     "          00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  0123456789ABCDEF");
    text = QString("%1:").arg(i, 8, 16, QLatin1Char('0'));
    y += step;
    for (; i < len; ) {
        text += QString(" %1").arg((uchar)m_data.at(i), 2, 16, QLatin1Char('0'));
        if ((uchar)m_data.at(i) < ' '
           || ((uchar)m_data.at(i) >= 0x7F && (uchar)m_data.at(i) <= 0xA0)
           || (uchar)m_data.at(i) == 0xAD)
        {
            ascii += "\u25A1";
        } else {
            ascii += m_data.at(i);
        }

        ++i;
        if (!(i & 0x0F)) {
            text += " |" + ascii + "|";
            painter.drawText(x, y, text);
            y += step;
            if (y > viewHeight) {goto _drawCoordinate;}
            ascii = "";
            text = QString("%1:").arg(i, 8, 16, QLatin1Char('0'));
        }
    }

    if ((len = ascii.length())) { //最后一行数据少于16个
        text += QString("%1 |%2|").arg(QString((16 - len) * 3, ' '), ascii);
        painter.drawText(x, y, text);
    }

_drawCoordinate:
    //绘制坐标
    i = 0; //当前选中数据可见标志
    pos = m_lineWidth / ROW_CHAR_COUNT * (m_currentColumn * 3 + 10) + x - 3; //当前列的位置
    painter.setPen(QColor(0xFF009900));
    if (pos > 0 && pos < viewWidth) {
        ++i;
        painter.drawLine(pos, 10, pos, viewHeight - 5);
    }
    pos = m_lineWidth / ROW_CHAR_COUNT * (10 + 48 + 1 + m_currentColumn) + x;
    if (pos > 0 && pos < viewWidth) {
        painter.drawLine(pos, 10, pos, viewHeight - 5);
    }
    pos = m_currentRow * step - this->verticalScrollBar()->value(); //当前行
    if (pos >= 0 && pos < viewHeight - step - m_lineHeight) {
        ++i;
        pos += m_lineHeight + ROW_SPACING + step + 5;
        painter.drawLine(0, pos, viewWidth - 5, pos);
    }

    if (i >= 2 && m_editVisible) {
        y = m_currentRow * step - this->verticalScrollBar()->value() + step + 10;
        step = m_lineWidth / ROW_CHAR_COUNT;
        x = (m_currentColumn * 3 + 10) * step - this->horizontalScrollBar()->value() - 2;
        m_edit->setVisible(true);
        m_edit->move(x, y);
    }else {
        m_edit->setVisible(false);
    }
}

void HexView::resizeEvent(QResizeEvent *e)
{
    this->adjustScrollRange();
    e->ignore();
}

void HexView::wheelEvent(QWheelEvent *e)
{
    if (this->verticalScrollBar()->isHidden()) {
        e->ignore();
    }else {
        int y, value;
        y = m_lineHeight + ROW_SPACING;
        value = this->verticalScrollBar()->value();
        if (e->angleDelta().y() > 0) {
            value -= y;
            if (value < 0) {
                value = 0;
            }
        }else {
            value += y;
            if (value > this->verticalScrollBar()->maximum()) {
                value = this->verticalScrollBar()->maximum();
            }
        }

        this->verticalScrollBar()->setValue(value);
        e->accept();
    }
}

void HexView::mousePressEvent(QMouseEvent *e)
{
    m_pos = e->pos();
    e->ignore();
}

void HexView::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->pos() == m_pos) {
        emit clicked(m_pos);
    }

    e->ignore();
}

void HexView::mouseDoubleClickEvent(QMouseEvent *)
{
    if (!m_edit->isHidden()) {
        m_edit->selectAll();
        m_edit->setFocus();
    }
}


void HexView::onEditingFinished()
{
    m_edit->setVisible(false);
    QString text = m_edit->text();
    if (text.isEmpty()) {return;}

    m_data[(m_currentRow << 4) + m_currentColumn] = text.toUInt(nullptr, 16);
    this->viewport()->update();
}

void HexView::onClicked(QPoint pos)
{
    int row, col, span, limit;
    col = pos.x() + this->horizontalScrollBar()->value();
    span = m_lineWidth / ROW_CHAR_COUNT;
    m_editVisible = false;
    m_edit->setVisible(false);
    if (col > span * 10 && col < span * 58) {
        col = (col / span - 10) / 3;
    }else {
        return;
    }

    span = m_lineHeight + ROW_SPACING;
    row = pos.y() - span - ROW_SPACING;
    limit = qMin(this->viewport()->height() - span,
               ((m_data.length() + 15) >> 4) * span - this->verticalScrollBar()->value()); //最大显示行结束坐标
    limit -= limit % span;

    if (row > 0 && row < limit) {
        row = (row + this->verticalScrollBar()->value()) / span;
    }else {
        return;
    }

    if ((row << 4) + col < m_data.length()) {
        m_currentColumn = col;
        m_currentRow = row;
        m_edit->setText(QString("%1").arg((uchar) m_data.at((m_currentRow << 4) + m_currentColumn), 2, 16, QLatin1Char('0')));
        m_editVisible = true;
        this->viewport()->update();
    }
}

void HexView::setData(const QByteArray &data)
{
    m_data = data;
    m_editVisible = false;
    m_edit->setVisible(false);
    this->adjustScrollRange();
    this->horizontalScrollBar()->setValue(0);
    this->verticalScrollBar()->setValue(0);

    this->viewport()->update();
}

void HexView::setBusy(bool b)
{
    this->blockSignals(b);
    if (b) {
        m_editVisible = false;
        m_edit->setVisible(false);
    }
}

void HexView::adjustScrollRange()
{
    QFontMetrics fm(this->font());
    int i, len, height;
    len = m_data.length();
    i = (len >> 4) + ((len & 0x0F) ? 1 : 0) + 2; //数据行数
    height = (m_lineHeight + ROW_SPACING) * i;
    if (m_lineWidth > this->viewport()->width()) {
        this->horizontalScrollBar()->setRange(0, m_lineWidth - this->viewport()->width());
    }else {
        this->horizontalScrollBar()->setRange(0, 0);
    }

    if (height > this->viewport()->height()) {
        this->verticalScrollBar()->setRange(0, height - this->viewport()->height());
    }else {
        this->verticalScrollBar()->setRange(0, 0);
    }
}
