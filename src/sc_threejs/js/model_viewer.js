import * as THREE from '../node_modules/three/build/three.module.js';
import { OBJLoader } from '../node_modules/three/examples/jsm/loaders/OBJLoader.js';
import { OrbitControls } from '../node_modules/three/examples/jsm/controls/OrbitControls.js';

// SCENE
var scene = new THREE.Scene();

// CAMERA
var camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);

// RENDERER
var renderer = new THREE.WebGLRenderer();
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

var controls = new OrbitControls( camera, renderer.domElement );

var animate = function () {
    requestAnimationFrame(animate);
    console.log(camera.position);
    light.position.copy(camera.position)
    controls.update();
    renderer.render(scene, camera);
};

animate();