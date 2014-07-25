package com.dbstar.bean;

public class OBean {
	private String right = "";
	private String left = "";
	private String back = "";
	private String up = "";
	private String down = "";

	public OBean(String up, String down, String left, String right, String back) {
		this.up = up;
		this.down = down;
		this.left = left;
		this.right = right;
		this.back = back;
	}

	public String getRight() {
		return right;
	}

	public void setRight(String right) {
		this.right = right;
	}

	public String getLeft() {
		return left;
	}

	public void setLeft(String left) {
		this.left = left;
	}

	public String getBack() {
		return back;
	}

	public void setBack(String back) {
		this.back = back;
	}

	public String getUp() {
		return up;
	}

	public void setUp(String up) {
		this.up = up;
	}

	public String getDown() {
		return down;
	}

	public void setDown(String down) {
		this.down = down;
	}

	public String toString() {
		return "up:" + this.getUp() + ",down:" + this.getDown() + ",left:"
				+ this.getLeft() + ",right:" + this.getRight() + ",back:"
				+ this.getBack();
	}
}