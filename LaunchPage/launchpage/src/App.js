import logo from './logo.svg';
import {HashRouter as Router, Routes, Route} from 'react-router-dom'
import { Home } from './Pages/home'
import { Settings } from './Pages/settings'
import './App.css';



function App() {
  return (
    <Router>
      <Routes>
        <Route path="/" element={<Home />}/>
        <Route path="/settings" element={<Settings />}/>
      </Routes>
    </Router>
 );
}

//      <div className="App">
//        <title>I4.0 Project</title>
//        <h1>I4.0 Project Dashboard</h1>
//        <header className="App-header">
//          <div className="App-grid" class="grid-container">
//            <button className='App-button' onClick={() => press()}>{'MQTT'}</button>
//            <button className='App-button' onClick={() => open_tab('http://192.168.10.2:1880')}>{'Node-Red'}</button>
//            <button className='App-button' onClick={() => open_tab('http://192.168.10.2:8088')}>{'Ignition'}</button>
//            <button className='App-button' onClick={() => press()}>{'Settings'}</button>
//          </div>
//        </header>
//      </div>
 
export default App;
