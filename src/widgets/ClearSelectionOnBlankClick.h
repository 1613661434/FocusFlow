#pragma once

class QAbstractItemView;
class QWidget;

void enableClearSelectionOnBlankClick(QAbstractItemView *view);
void enableClearSelectionOnClick(QWidget *surface, QAbstractItemView *view);
