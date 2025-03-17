# I should pin the nixpkgs import
{ pkgs ? import <nixpkgs> { } }:
pkgs.mkShell 
{
	name = "C";
	buildInputs = with pkgs;
	[
	];
	
	# Any name not reserved will be interpreted as a shell variable
	# Some shell variables are protected and to change them one must use a shellHook
	# shellHook is a way to run shell commands at startup
	#shellHook = ''
	
	hardeningDisable = ["all"];
}
