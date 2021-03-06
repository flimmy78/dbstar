package com.dbstar.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;

import com.dbstar.R;

public class GDCircleTextView extends View {
	private float mRadius;
	private float mStartAngle, mSweepAngle;
	private int mTextColor;
	private float mTextSize;
	private String mText;

	private Path mCircle;
	private Paint mTPaint;

	Paint cPaint;

	float mHOffset;
	float mVOffset;
	public GDCircleTextView(Context context) {
		this(context, null);
	}

	public GDCircleTextView(Context context, AttributeSet attrs) {
		super(context, attrs);

		TypedArray a = context.obtainStyledAttributes(attrs,
				R.styleable.GDCircleTextView);
		setLayerType(View.LAYER_TYPE_SOFTWARE, null);
		mRadius = a.getFloat(R.styleable.GDCircleTextView_circleRadius, 0);
		mStartAngle = a.getFloat(R.styleable.GDCircleTextView_startAngle, 0);
		mSweepAngle = a.getFloat(R.styleable.GDCircleTextView_sweepAngle, 0);
		mTextColor = a.getColor(R.styleable.GDCircleTextView_textColor, 0);
		mTextSize = a.getDimension(R.styleable.GDCircleTextView_textSize, 12);
		mText = a.getString(R.styleable.GDCircleTextView_text);
		mHOffset = a.getDimension(R.styleable.GDCircleTextView_hOffset, 0);
		mVOffset = a.getDimension(R.styleable.GDCircleTextView_vOffset, 0);
		a.recycle();

		setFocusable(false);

		mTPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
		mTPaint.setStyle(Paint.Style.FILL_AND_STROKE);
		mTPaint.setColor(mTextColor);
		mTPaint.setTextAlign(Paint.Align.CENTER);
		mTPaint.setTextSize(mTextSize);

		cPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
		cPaint.setStyle(Paint.Style.STROKE);
		cPaint.setColor(Color.BLACK);
		cPaint.setStrokeWidth(1);
	}

	public void setText(String text) {
		mText = text;
		invalidate();
	}
	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		super.onMeasure(widthMeasureSpec, heightMeasureSpec);
	}

	@Override
	protected void onLayout(boolean changed, int left, int top, int right,
			int bottom) {
		super.onLayout(changed, left, top, right, bottom);
	}

	@Override
	protected void onDraw(Canvas canvas) {
		// super.onDraw(canvas);

		if (mCircle == null) {
		    mCircle = new Path();
			 //float y = mRadius;
			 //float x = getWidth() / 2;
			 //mCircle.addCircle(x, y, mRadius, Direction.CW);
			RectF oval = new RectF(0, 0, getWidth(), getHeight());
			mCircle.addArc(oval, mStartAngle, mSweepAngle);
			//mHOffset = (float)(Math.PI*mRadius/180*mStartAngle);
//			RectF oval = new RectF(0, 0, mRadius, mRadius);
//			mCircle.addArc(oval, mStartAngle, mSweepAngle);
		}

		//canvas.drawPath(mCircle, cPaint);

		if (mText != null && !mText.isEmpty()) {
			canvas.drawTextOnPath(mText, mCircle, mVOffset, mHOffset, mTPaint);
		}
	}
	
	public void setValue(float startAngle,float sweepAngle,String textValue){
	    mStartAngle = startAngle;
	    mSweepAngle = sweepAngle;
	    mText = textValue;
	    mCircle = null;
	    invalidate();
	}
}
