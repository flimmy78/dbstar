package com.dbstar.guodian.model;

import java.util.List;

public class ContentData {
	public String Id;
	public String XMLFilePath;

	public String Name;
	public String Type;
	public String Keywords;
	public String Actors;
	public String Director;
	public String Area;
	public String Source;
	public String SubTitle;
	public String Language;
	
	public String Description;
	
	public String ImageDefinition;
	public String Resolution;
	public String AudioChannel;
	public String AspectRatio;
	public String Audience;
	public String Model;
	public int Length;

	public String DRMFile;
	public List<Poster> Posters; 
	public List<Trailer> Trailers;
	public List<SubTitle> SubTitles;
	
	public MFile MainFile;
	
	public static class SubTitle {
		public String Id;
		public String Name;
		public String Language;
		public String URI;
	}
	
	public static class Trailer {
		public String Id;
		public String Name;
		public String URI;
	}
	
	public static class Poster {
		public String Id;
		public String Name;
		public String URI;
	}
	
	public static class MFile {
		public String FileID;
		public String FileName;
		public String FileType;
		public String FileSize;
		public String Duration;
		public String FileURI;
		public String Resolution;
		public String BitRate;
		public String FileFormat;
		public String CodeFormat;
	}
}