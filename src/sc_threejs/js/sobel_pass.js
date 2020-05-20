import * as THREE from '../node_modules/three/build/three.module.js';
import { OBJLoader } from '../node_modules/three/examples/jsm/loaders/OBJLoader.js';
// import { OrbitControls } from '../node_modules/three/examples/jsm/controls/OrbitControls.js';
import { TrackballControls } from '../node_modules/three/examples/jsm/controls/TrackballControls.js'
import Stats from '../node_modules/three/examples/jsm/libs/stats.module.js'
import GUI from '../node_modules/three/examples/jsm/libs/dat.gui.module.js'

import { EffectComposer } from '../node_modules/three/examples/jsm/postprocessing/EffectComposer.js';
import { RenderPass } from '../node_modules/three/examples/jsm/postprocessing/RenderPass.js';
import { ShaderPass } from '../node_modules/three/examples/jsm/postprocessing/ShaderPass.js';
import { LuminosityShader } from '../node_modules/three/examples/jsm/shaders/LuminosityShader.js';

var camera, scene, renderer, composer;
var effectSobel;

var SobelOperatorShader = {
	uniforms: {

		"tDiffuse": { value: null },
		"resolution": { value: new THREE.Vector2() }

	},

	vertexShader: [

		"varying vec2 vUv;",

		"void main() {",

		"	vUv = uv;",

		"	gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );",

		"}"

	].join( "\n" ),

	fragmentShader: [

		"uniform sampler2D tDiffuse;",
		"uniform vec2 resolution;",
		"varying vec2 vUv;",

		"void main() {",

		"	vec2 texel = vec2( 1.0 / resolution.x, 1.0 / resolution.y );",

		// kernel definition (in glsl matrices are filled in column-major order)

		"	const mat3 Gx = mat3( -1, -2, -1, 0, 0, 0, 1, 2, 1 );", // x direction kernel
		"	const mat3 Gy = mat3( -1, 0, 1, -2, 0, 2, -1, 0, 1 );", // y direction kernel

		// fetch the 3x3 neighbourhood of a fragment

		// first column

		"	float tx0y0 = texture2D( tDiffuse, vUv + texel * vec2( -1, -1 ) ).r;",
		"	float tx0y1 = texture2D( tDiffuse, vUv + texel * vec2( -1,  0 ) ).r;",
		"	float tx0y2 = texture2D( tDiffuse, vUv + texel * vec2( -1,  1 ) ).r;",

		// second column

		"	float tx1y0 = texture2D( tDiffuse, vUv + texel * vec2(  0, -1 ) ).r;",
		"	float tx1y1 = texture2D( tDiffuse, vUv + texel * vec2(  0,  0 ) ).r;",
		"	float tx1y2 = texture2D( tDiffuse, vUv + texel * vec2(  0,  1 ) ).r;",

		// third column

		"	float tx2y0 = texture2D( tDiffuse, vUv + texel * vec2(  1, -1 ) ).r;",
		"	float tx2y1 = texture2D( tDiffuse, vUv + texel * vec2(  1,  0 ) ).r;",
		"	float tx2y2 = texture2D( tDiffuse, vUv + texel * vec2(  1,  1 ) ).r;",

		// gradient value in x direction

		"	float valueGx = Gx[0][0] * tx0y0 + Gx[1][0] * tx1y0 + Gx[2][0] * tx2y0 + ",
		"		Gx[0][1] * tx0y1 + Gx[1][1] * tx1y1 + Gx[2][1] * tx2y1 + ",
		"		Gx[0][2] * tx0y2 + Gx[1][2] * tx1y2 + Gx[2][2] * tx2y2; ",

		// gradient value in y direction

		"	float valueGy = Gy[0][0] * tx0y0 + Gy[1][0] * tx1y0 + Gy[2][0] * tx2y0 + ",
		"		Gy[0][1] * tx0y1 + Gy[1][1] * tx1y1 + Gy[2][1] * tx2y1 + ",
		"		Gy[0][2] * tx0y2 + Gy[1][2] * tx1y2 + Gy[2][2] * tx2y2; ",

		// magnitute of the total gradient

		"	float G = sqrt( ( valueGx * valueGx ) + ( valueGy * valueGy ) );",

		"	gl_FragColor = vec4( vec3( G ), 1 );",

		"}"

	].join( "\n" )
};

// STATS
var stats = new Stats();
document.body.appendChild(stats.domElement);

// SCENE
scene = new THREE.Scene();

// CAMERA
camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
scene.add(camera)

// RENDERER
renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

// MODEL LOADER
var loader = new OBJLoader();
var themat = new THREE.MeshPhongMaterial( { color: 0x0033ff, specular: 0x555555, shininess: 30 } );

loader.load(
	// resource URL
	"models/bunny.obj",

	// onLoad callback
	// Here the loaded data is assumed to be an object
	function ( obj ) {
        // Add the loaded object to the scene
        obj.children[0].material = themat;
		scene.add( obj );
	},

	// onProgress callback
	function ( xhr ) {
		console.log( (xhr.loaded / xhr.total * 100) + '% loaded' );
	},

	// onError callback
	function ( err ) {
		console.error( 'An error happened' );
	}
);

camera.position.z = 5;

var light = new THREE.PointLight( 0x2047ff, 50, 0 );
light.position.set(0,0,5);
scene.add( light );

// TRACKBALL CONTROLS
var controls = new TrackballControls( camera, renderer.domElement );

// AXIS
var axesHelper = new THREE.AxesHelper( 5 );
scene.add( axesHelper );

// RENDER PASS
composer = new EffectComposer( renderer );
var renderPass = new RenderPass( scene, camera );
composer.addPass( renderPass );
var effectGrayScale = new ShaderPass( LuminosityShader );
composer.addPass( effectGrayScale );
composer.setSize( window.innerWidth, window.innerHeight );

var effectSobel = new ShaderPass( SobelOperatorShader );
effectSobel.uniforms[ 'resolution' ].value.x = window.innerWidth * window.devicePixelRatio;
effectSobel.uniforms[ 'resolution' ].value.y = window.innerHeight * window.devicePixelRatio;
composer.addPass( effectSobel );

// GUI
// todo

var animate = function () {
    requestAnimationFrame(animate);
    light.position.copy(camera.position)
    controls.update();
    stats.update();
    composer.render(scene, camera);
};

animate();